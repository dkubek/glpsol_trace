#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <vector>

#include "Graph.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

const Graph::CNumber C_INF = Graph::Inf<Graph::CNumber>();
const Graph::FNumber F_INF = Graph::Inf<Graph::FNumber>();

bool
is_unified_cost_arc(Graph& graph, Graph::Index j)
{
    auto first = graph.CostKJ(0, j);

    for (Graph::Index k = 1; k < graph.NrComm(); ++k) {
        if (graph.CostKJ(k, j) != first)
            return false;
    }

    return true;
}

bool
is_unified_cost(Graph& graph)
{
    if (graph.NrComm() <= 1) {
        return true;
    }

    for (Graph::Index j = 0; j < graph.NrArcs(); j++) {
        if (!is_unified_cost_arc(graph, j))
            return false;
    }

    return true;
}

void
collect_info(Graph& graph, json& j)
{
    auto info = json::object();
    info["no_nodes"] = graph.NrNodes();
    info["no_arcs"] = graph.NrArcs();
    info["no_commodities"] = graph.NrComm();

    j["info"] = info;
}

void
collect_arcs(Graph& graph, json& j)
{
    auto start_nodes = graph.StartN();
    auto end_nodes = graph.EndN();
    auto mutual_capacities = graph.TotCapacities();

    int fix = 0;
    if (graph.NamesStartFrom1())
        fix = 1;

    auto arcs = json::array();
    for (Graph::Index i = 0; i < graph.NrArcs(); i++) {

        Graph::FNumber sum_capacity{ 0 };
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto capacity = graph.CapacityKJ(k, i);
            sum_capacity += capacity;

            if (sum_capacity >= F_INF)
                break;
        }

        auto arc = json::object();
        arc["from"] = start_nodes[i] - fix;
        arc["to"] = end_nodes[i] - fix;

        auto arc_capacity = std::min(sum_capacity, mutual_capacities[i]);
        if (arc_capacity == F_INF)
            arc["capacity"] = "null";
        else
            arc["capacity"] = arc_capacity;

        if (is_unified_cost_arc(graph, i))
            arc["cost"] = graph.CostKJ(0, i);
        else {
            auto costs = json::object();
            for (Graph::Index k = 0; k < graph.NrComm(); k++) {
                auto cost = graph.CostKJ(k, i);

                auto key = std::to_string(k);
                if (cost < C_INF) {
                    costs[key] = cost;
                } else {
                    costs[key] = "null";
                }
            }

            arc["cost"] = graph.CostKJ(0, i);
        }

        arcs.push_back(arc);
    }

    j["arcs"] = arcs;
}

void
collect_demands(Graph& graph, json& j)
{
    auto demands = json::object();
    for (Graph::Index i = 0; i < graph.NrNodes(); i++) {

        auto tmp = json::object();
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto deficit = graph.DeficitKJ(k, i);
            if (deficit != 0) {
                tmp[std::to_string(k)] = deficit;
            }
        }

        demands[std::to_string(i)] = tmp;
    }

    j["demands"] = demands;
}

std::ostream&
print_network_json(std::ostream& os, Graph& graph)
{
    json j;

    collect_info(graph, j);
    collect_arcs(graph, j);
    collect_demands(graph, j);

    os << j << std::endl;

    return os;
}

void
print_help()
{
    std::cout
      << "Usage: convert_problem [OPTIONS]\n"
         "\nOptions:\n"
         "  -h, --help             Display this help message and exit\n"
         "  -i, --input <file>     Specify input filename\n"
         "  -f, --format <type>    Specify format type (s/c/p/o/d/u/m)\n"
         "  -c, --check-cost       Enable checking unified cost\n"
         "  -o, --output <file>    Specify output filename\n"
         "  -t, --out-format <fmt> Specify output format mps/json (default: mps) \n";
}

int
main(int argc, char* argv[])
{
    int opt;
    std::string input_filename;
    char format_type = '\0';
    bool check_unified_cost = false;
    std::string output_filename;
    std::string output_format = "mps";

    static struct option long_options[] = {
        { "help", no_argument, nullptr, 'h' },
        { "input", required_argument, nullptr, 'i' },
        { "format", required_argument, nullptr, 'f' },
        { "check-cost", no_argument, nullptr, 'c' },
        { "output", required_argument, nullptr, 'o' },
        { "out-format", required_argument, nullptr, 't' },
        { nullptr, 0, nullptr, 0 }
    };

    int long_index = 0;
    while ((opt = getopt_long(
              argc, argv, "hi:f:co:t:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'i':
                input_filename = std::string(optarg);
                break;
            case 'f':
                format_type = optarg[0];
                if (std::string("scpodum").find(format_type) ==
                    std::string::npos) {
                    std::cerr << "Error: Invalid format type\n";
                    print_help();
                    return 1;
                }
                break;
            case 'c':
                check_unified_cost = true;
                break;
            case 'o':
                output_filename = std::string(optarg);
                break;
            case 't':
                output_format = std::string(optarg);
                if (output_format != "json" && output_format != "mps" &&
                    output_format != "info") {
                    std::cerr << "Error: Invalid output format\n";
                    print_help();
                    return 1;
                }
                break;
            default:
                print_help();
                return 1;
        }
    }

    if (input_filename.empty() || format_type == '\0') {
        std::cerr << "Error: Missing required arguments\n";
        print_help();
        return 1;
    }

    // Read the problem
    Graph graph(input_filename.c_str(), format_type);

    graph.MakeSingleSourced();
    graph.PreProcess();

    // Check cost and exit
    if (check_unified_cost) {
        if (!is_unified_cost(graph)) {
            std::cout << "Problem " << input_filename
                      << " does NOT have unified costs!\n";
            return EXIT_FAILURE;
        } else {
            std::cout << "Problem " << input_filename
                      << " does have unified costs.\n";
            return EXIT_SUCCESS;
        }
    }

    if (output_format == "json") {
        if (!output_filename.empty()) {
            {
                std::ofstream fout{ output_filename };
                print_network_json(fout, graph);
            }
        } else {
            print_network_json(std::cout, graph);
        }
    } else if (output_format == "mps") {

        if (output_filename.empty()) {
            std::cerr << "Output file needs to be specified for MPS!"
                      << std::endl;
            print_help();
            return EXIT_FAILURE;
        }

        graph.OutMPSFile(output_filename.c_str());
    } else if (output_format == "info") {
        std::cout << graph.NrNodes() << ' ' << graph.NrArcs() << ' '
                  << graph.NrComm() << std::endl;
    }

    return EXIT_SUCCESS;
}
