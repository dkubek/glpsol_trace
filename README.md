# glpsol_trace

This little C++ project demonstrates how to use the
[dkubek/glpk-simplex-trace][glpk-simplex-trace] library. The project contains
two tools. The ``glpsol_trace`` tool used for solving problems in exact
arithmetic with tracing and ``convert_problem`` used to convert MMCF problem
instances.

[glpk-simplex-trace]: https://github.com/dkubek/glpk-simplex-trace

## Installation

### Dependencies

To build this project, you need to have the following dependencies installed:

- Git
- CMake (version at least 3.11)
- A C++ compiler (such as g++)
- The GNU MP library

### Building the Project

1. Clone the repository with all of its submodules:
```
git clone --recursive https://github.com/dkubek/glpsol_trace.git
cd glpsol_trace
```

2. Configure the build
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cd build
cmake --build .
```

### Using Docker

#### Building the Docker Image

You can build the Docker image with the following command:

```
docker build -t glpsol_trace .
```

#### Running the Pre-built Docker Image

You can also run the pre-built version of the project using Docker:
```
docker run --rm -it dkubek/glpsol_trace "<CMD>"
```
Note the quotes ``""`` in the above command.

## Usage

### ``glpsol_trace``
```sh
glpsol_trace <MODEL_FILENAME> [OPTION...] 
```

The available options are:
 - ``--lp``: Specifies the input type of ``MODEL_FILENAME`` to be the CPLEX LP format
 - ``--mps``: Specifies the input type of ``MODEL_FILENAME`` to be (FREE) MPS format
 - ``--pivot <RULE>``: Specifies the pivoting rule to use. Available options for ``RULE`` are ``dantzig``, ``bland``, ``best`` and ``random``
 - ``--info-file <FILENAME>``: Specifies file where to store problem information and final solution.
 - ``--obj-file <FILENAME>``: Specifies file where to store the trace of objective values.
 - ``--status-file <FILENAME>``: Specifies file where to store the trace of variable status.
 - ``--var-file <FILENAME>``: Specifies file where to store the trace of variable values.
 - ``-h, --help``: Print usage

Additional options used mostly for internal purposes are:
 - ``--info``: Print problem info and exit
 - ``--bits-only``: Print the number of fractionality bits only
 - ``--scale``: Scale the problem to have integral RHS.

### ``convert_problem``

```
convert_problem [OPTIONS]
```

The available options are:
 -  ``-i, --input <file>``:     Specify input filename
 -  ``-f, --format <type>``:    Specify format type (s/c/p/o/d/u/m) for the [Graph library][graph]
 -  ``-o, --output <file>``:    Specify output filename
 -  ``-t, --out-format <fmt>``: Specify output format ``mps``/``json`` (default: mps)
 - ``-h, --help``             Display this help message and exit

One additional option used for internal purposes is:
  ``-c, --check-cost``       Enable checking unified cost. This checks whether every arc has the same cost for all commodities.

[graph]: http://groups.di.unipi.it/optimize/Software/Graph.html

## Example

In the ``test/`` directory we have provided the archive ``sample.tar.gz`` containing a sample of small MMCF problems from the [MMCF Dataset][mmcf].

Firstly, unpack the archive
```sh
tar -zxvf sample.tar.gz
```
this creates the ``sample/`` directory and populates it with the problems. Change into it.
```
cd sample/
```

We
have also provided a small script ``process`` for converting the
instances into the MPS format read by ``glpsol_trace``. 


If the ``convert_problem`` executable is available in ``PATH`` run
```sh
./process all
```
otherwise, run the script as with the ``CONVERT_SCRIPT`` variable set
```sh
CONVERT_SCRIPT=/path/to/convert_problem ./process all
```
This creates a MPS file for each problem.


To for example solve the ``mnetgen/64-4-1.mps`` problem  using the dantzig rule
run:
```sh
glpsol_trace mnetgen/64-4-1.mps \
  --mps \
  --pivot dantzig \
  --info-file info \
  --status-file status \
  --var-file var
```

[mmcf]: http://groups.di.unipi.it/optimize/Data/MMCF.html#vnc
