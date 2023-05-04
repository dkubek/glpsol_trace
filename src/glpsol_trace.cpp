#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#define HAVE_GMP
#include <glpk.h>

#include <cstring>
#include <cxxopts.hpp>

#define MODEL_LP 1
#define MODEL_MPS 2

constexpr glp_smcp DEFAULT_GLP_SMCP = {
    .msg_lev = GLP_MSG_ALL,
    .meth = GLP_PRIMAL,
    .pricing = GLP_PT_PSE,
    .r_test = GLP_RT_STD,
    .tol_bnd = 1e-7,
    .tol_dj = 1e-7,
    .tol_piv = 1e-9,
    .obj_ll = -std::numeric_limits<double>::max(),
    .obj_ul = std::numeric_limits<double>::max(),
    .it_lim = std::numeric_limits<int>::max(),
    .tm_lim = std::numeric_limits<int>::max(),
    .out_frq = 500,
    .out_dly = 0,
    .presolve = GLP_OFF
};

constexpr glp_stmcp GLP_DEFAULT_STMCP = {
    .store_mem = GLP_STORE_TRACE_MEM_OFF,

    .objective_trace = GLP_OBJECTIVE_TRACE_ON,
    .basis_trace = GLP_BASIS_TRACE_ON,
    .status_trace = GLP_STATUS_TRACE_ON,

    .pivot_rule = GLP_TRACE_PIVOT_DANTZIG,

    .fractionality_bits_trace = GLP_TRACE_BITS_ONLY_OFF,
    .scale = GLP_TRACE_SCALE_OFF,

    .info_file_basename = { '\0' },
    .objective_values_file_basename = { '\0' },
    .status_file_basename = { '\0' },
    .variable_values_file_basename = { '\0' },
};

void
read_problem(const std::string& filename, int format, glp_prob* P)
{ // Read the problem from a file
    if (format == MODEL_LP) {
        glp_read_lp(P, nullptr, filename.c_str());
    } else if (format == MODEL_MPS) {
        glp_read_mps(P, GLP_MPS_FILE, nullptr, filename.c_str());
    } else {
        glp_assert(format != format);
    }
}

std::ostream&
print_info_and_exit(std::ostream& os, const std::string& filename, int format)
{
    glp_prob* P;
    P = glp_create_prob();

    // Read the problem from a file
    read_problem(filename, format, P);

    auto ncols = glp_get_num_cols(P);
    auto nrows = glp_get_num_rows(P);
    auto nonzeros = glp_get_num_nz(P);

    os << "rows: " << nrows << '\n';
    os << "cols: " << ncols << '\n';
    os << "nonzeros: " << nonzeros << '\n';

    return os;
}

std::ostream&
solve_with_trace(std::ostream& os,
                 const std::string& problem_filename,
                 int problem_file_format,
                 const std::string& info_filename,
                 const std::string& objective_filename,
                 const std::string& variable_filename,
                 const std::string& status_filename,
                 int pivot_rule,
                 int fractionality_bits_only,
                 int scale)
{
    glp_prob* P;
    P = glp_create_prob();

    glp_smcp params = DEFAULT_GLP_SMCP;
    params.msg_lev = GLP_MSG_ERR;

    // Read the problem from a file
    read_problem(problem_filename, problem_file_format, P);

    // Construct initial basis
    glp_adv_basis(P, 0);

    // Set tracing parameters
    auto trace_params = GLP_DEFAULT_STMCP;
    strcpy(trace_params.status_file_basename, status_filename.c_str());
    strcpy(trace_params.objective_values_file_basename,
           objective_filename.c_str());
    strcpy(trace_params.variable_values_file_basename,
           variable_filename.c_str());
    strcpy(trace_params.info_file_basename, info_filename.c_str());
    trace_params.pivot_rule = pivot_rule;
    trace_params.fractionality_bits_trace = fractionality_bits_only;
    trace_params.scale = scale;

    // Create the trace object
    auto trace = glp_create_ssxtrace(&trace_params);

    // Solve the problem and trace parameters
    glp_exact_trace(P, &params, trace);

    os << "DONE" << std::endl;

    // Cleanup
    glp_ssxtrace_free(trace);
    glp_delete_prob(P);

    return os;
}

int
main(int argc, char* argv[])
{
    cxxopts::Options options(
      "glpsol_trace", "Solve a linear program and trace feasible solutions.");

    options.add_options()
      ("model-file", "Linear programming problem file", cxxopts::value<std::string>())
      ("lp", "Model file is in LP format")("mps", "Model file is in MPS format")
      ("pivot", "Pivoting rule to use. Available : dantzig, bland, best, random", cxxopts::value<std::string>())
      ("info-file", "File where to store problem information and final solution.", cxxopts::value<std::string>())
      ("obj-file", "File where to store the trace of objective values.", cxxopts::value<std::string>())
      ("status-file", "File where to store the trace of variable status.", cxxopts::value<std::string>())
      ("var-file", "File where to store the values of variables.", cxxopts::value<std::string>())
      ("info", "Print problem info and exit")
      ("bits-only", "Print the number of fractionality bits")
      ("scale", "Scale the problem to have integral RHS.")
      ("h,help", "Print usage");

    options.parse_positional({ "model-file" });
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    std::string problem_filename;
    int problem_file_format = MODEL_LP;

    std::string info_filename;
    std::string objective_filename;
    std::string status_filename;
    std::string variable_filename;
    if (result.count("model-file")) {
        problem_filename = result["model-file"].as<std::string>();
    }
    else  {
        std::cerr << "Need to specify the model file!" << '\n';
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    if (result.count("lp"))
        problem_file_format = MODEL_LP;
    if (result.count("mps"))
        problem_file_format = MODEL_MPS;

    if (result.count("info-file"))
        info_filename = result["info-file"].as<std::string>();
    if (result.count("obj-file"))
        objective_filename = result["obj-file"].as<std::string>();
    if (result.count("status-file"))
        status_filename = result["status-file"].as<std::string>();
    if (result.count("var-file"))
        variable_filename = result["var-file"].as<std::string>();

    int pivot_rule;
    if (!result.count("pivot")) {
        std::cerr << "Need to specify pivoting rule!" << '\n';
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    auto pivot_rule_name = result["pivot"].as<std::string>();
    if (pivot_rule_name == "dantzig") {
        pivot_rule = GLP_TRACE_PIVOT_DANTZIG;
    } else if (pivot_rule_name == "bland") {
        pivot_rule = GLP_TRACE_PIVOT_BLAND;
    } else if (pivot_rule_name == "best") {
        pivot_rule = GLP_TRACE_PIVOT_BEST;
    } else if (pivot_rule_name == "random") {
        pivot_rule = GLP_TRACE_PIVOT_RANDOM;
    } else {
        std::cerr << "Unknown pivoting rule: " << pivot_rule_name << " !"
                  << '\n';
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    int fractionality_bits_only = GLP_TRACE_BITS_ONLY_OFF;
    if (result.count("bits-only")) {
        fractionality_bits_only = GLP_TRACE_BITS_ONLY_ON;
    }

    int scale = GLP_TRACE_BITS_ONLY_OFF;
    if (result.count("scale")) {
        scale = GLP_TRACE_BITS_ONLY_ON;
    }

    if (result.count("info")) {
        print_info_and_exit(std::cout, problem_filename, problem_file_format);
        return EXIT_SUCCESS;
    }

    solve_with_trace(std::cout,
                     problem_filename,
                     problem_file_format,
                     info_filename,
                     objective_filename,
                     variable_filename,
                     status_filename,
                     pivot_rule,
                     fractionality_bits_only,
                     scale);

    return EXIT_SUCCESS;
}
