#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

#include "Graph.h"

const Graph::CNumber C_INF = Graph::Inf<Graph::CNumber>();
const Graph::FNumber F_INF = Graph::Inf<Graph::FNumber>();


bool is_unified_cost(Graph &graph) {
    if (graph.NrComm() <= 1) { return true; }

    for (Graph::Index j = 0; j < graph.NrArcs(); j++) {
        auto first = graph.CostKJ(0, j);

        for (Graph::Index k = 1; k < graph.NrComm(); ++k) {
            if (graph.CostKJ(k, j) != first) return false;
        }
    }

    return true;
}

std::ostream &print_network_json(std::ostream &os, Graph &graph) {
    os << std::boolalpha;
    os << "{" << '\n';

    os << "  \"info\" : {" << '\n';
    os << "    \"no_nodes\": " << graph.NrNodes() << "," << '\n';
    os << "    \"no_arcs\": " << graph.NrArcs() << "," << '\n';
    os << "    \"no_commodities\": " << graph.NrComm() << '\n';
    os << "  }," << '\n';

    auto start_nodes = graph.StartN();
    auto end_nodes = graph.EndN();
    auto mutual_capacities = graph.TotCapacities();

    os << "  \"arcs\" : [" << '\n';
    for (Graph::Index i = 0; i < graph.NrArcs(); i++) {
        if (i) os << ",";

        Graph::FNumber sum_capacity{0};
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto capacity = graph.CapacityKJ(k, i);
            sum_capacity += capacity;

            if (sum_capacity >= F_INF) break;
        }

        os << "    { ";
        os << "\"from\": " << start_nodes[i] << ", \"to\": " << end_nodes[i]
           << ",";
        auto arc_capacity = std::min(sum_capacity, mutual_capacities[i]);
        os << " \"capacity\": ";
        os << (arc_capacity == F_INF ? -1 : arc_capacity);
        os << "," << '\n';
        os << "      \"cost\": {";

        bool printed = false;
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            if (printed) os << ',';

            auto cost = graph.CostKJ(k, i);
            if (cost < C_INF) {
                os << '"' << k << '"' << " : " << cost;
            } else {
                os << '"' << k << '"' << " : "
                   << "null";
            }
            printed = true;
        }
        os << "}" << '\n';
        os << "    }";
    }
    os << '\n' << "  ]," << '\n';

    os << "  \"demands\": {";
    for (Graph::Index i = 0; i < graph.NrNodes(); i++) {
        if (i) os << ',';

        bool printed = false;
        os << '\"' << (i + 1) << "\": {"
           << "\n";
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto deficit = graph.DeficitKJ(k, i);
            if (deficit != 0) {
                if (printed) os << ',';

                os << "      \"" << k << "\": " << deficit << "\n";
                printed = true;
            }
        }
        os << "    }";
    }
    os << '\n' << "  }" << '\n';

    os << "}" << '\n';

    return os;
}

std::ostream &print_network(std::ostream &os, Graph &graph) {
    os << "{" << '\n';

    os << "\"info\" : " << '\n';
    os << "{" << '\n';
    os << "\"no_nodes\": " << graph.NrNodes() << ",\n";
    os << "\"no_arcs\": " << graph.NrArcs() << ",\n";
    os << "\"no_commodities\": " << graph.NrComm() << "\n";
    os << "}," << '\n';

    // Output all arcs and their capacities
    auto start_nodes = graph.StartN();
    auto end_nodes = graph.EndN();
    auto mutual_capacities = graph.TotCapacities();

    os << "\"arcs\" : " << '\n';
    os << "[" << '\n';
    for (Graph::Index i = 0; i < graph.NrArcs(); i++) {

        if (i) os << ',';

        // Sum capacities over all commodities
        Graph::CNumber sum_capacity{0};
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto capacity = graph.CapacityKJ(k, i);
            sum_capacity += capacity;

            if (sum_capacity >= F_INF) break;
        }

        os << "{ ";
        os << " \"from\":" << start_nodes[i] << ", \"to\": " << end_nodes[i]
           << ",";

        auto arc_capacity = std::min(sum_capacity, mutual_capacities[i]);
        os << "\"capacity\" :";
        if (arc_capacity == F_INF) {
            os << -1;
        } else {
            os << arc_capacity;
        }
        os << ",\n";
        os << "\"cost\" : {";
        auto printed = false;
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            if (printed) os << ',';

            auto cost = graph.CostKJ(k, i);
            if (cost < C_INF) {
                os << '"' << k << '"' << " : " << cost;
            } else {
                os << '"' << k << '"' << " : "
                   << "null";
            }
            printed = true;
        }
        os << "}\n";
        os << "}";
    }
    os << "]," << '\n';

    os << "\"demands\": {";
    // Print information about the deficits of nodes
    for (Graph::Index i = 0; i < graph.NrNodes(); i++) {
        if (i) os << ',';

        auto printed = false;
        os << '\"' << (i + 1) << '\"' << ": { "
           << "\n";
        for (Graph::Index k = 0; k < graph.NrComm(); k++) {
            auto deficit = graph.DeficitKJ(k, i);
            if (deficit != 0) {
                if (printed) os << ',';

                os << '\t' << '\"' << k << '\"' << " : " << deficit << "\n";
                printed = true;
            }
        }
        os << "}" << '\n';
    }
    os << "}";

    os << "}" << '\n';

    return os;
}

void print_help() {
    std::cout << "Usage: my_program [options] -i <input_filename> -f "
                 "<format_type>\n"
              << "Options:\n"
              << "  -i <input_filename>   Input file name (string)\n"
              << "  -f <format_type>      Format type (char): s, c, p, o, d, "
                 "u, m\n"
              << "  -c                    Check unified cost (optional flag)\n"
              << "  -o <output_type>      Output type: 'json' for JSON, "
                 "'other' for other format\n"
              << "  -h                    Print help\n";
}

int main(int argc, char *argv[]) {
    int opt;
    std::string input_filename;
    char format_type;
    bool check_unified_cost = false;
    std::string output_type = "json";

    while ((opt = getopt(argc, argv, "hi:f:co:")) != -1) {
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
                output_type = std::string(optarg);
                if (output_type != "json" && output_type != "network") {
                    std::cerr << "Error: Invalid output type\n";
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

    // graph.PreProcess();

    // Check cost and exit
    if (check_unified_cost) {
        if (!is_unified_cost(graph)) {
            std::cout << "Problem " << input_filename
                      << " does NOT have unified costs!\n";
            return EXIT_FAILURE;
        }
        else {
            std::cout << "Problem " << input_filename
                      << " does have unified costs.\n";
            return EXIT_SUCCESS;
        }
    }

    if (output_type == "json") {
        print_network_json(std::cout, graph);
    }
    else if (output_type == "network") {
        print_network(std::cout, graph);
    } else {
        std::cerr << "Output type: " << output_type << " not implemented!";
    }

    return EXIT_SUCCESS;
}