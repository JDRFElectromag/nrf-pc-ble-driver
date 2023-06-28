#!/bin/bash

printHelp ()
{
    echo "###################### Commands ######################"
    echo "### -c,  --clean     -> Clean all target builds files for the selected project(s)"
    echo "### -b,  --build     -> Build all targets for the selected project(s)"
    exit 1
}

##################################################################
# Defaults
##################################################################
rootDir=$(git rev-parse --show-toplevel)
autoBuild=0
clean=0
buildRootDir="${rootDir}/build"

##################################################################
# Input Argument Parsing
##################################################################
for ((i=1; i<=$#; i++)) do
    arg=${!i}
    case "$arg" in
        -b|--build)
            autoBuild=1
            ;;
        -c|--clean)
            clean=1
            ;;
        help|-h|--help|-help)
            printHelp
            ;;
        *)
            echo "Unknown input \"$arg\""
            printHelp
            ;;
    esac
done

##################################################################
# Method
##################################################################
getCpuCores ()
{
    cpuCores=8 #Default
    local rc=1
    local os=$(uname)
    if [ "Linux" == "${os}" ]; then
        local cores=`nproc --all`
        if [ $? -ne 0 ]; then
            cpuCores=${cores}
        fi
    fi
}

generateCmakeProject ()
{
    mkdir -p ${buildRootDir}
    pushd ${buildRootDir}
    cmake -G Ninja ${rootDir}
    popd
}

checkAndClean ()
{
    if [ ${clean} -eq 1 ]; then
        echo "Cleaning: Removing ${buildRootDir}"
        rm -rf ${buildRootDir}
    fi
}

checkAndBuild ()
{
    if [ ${autoBuild} -eq 1 ]; then
        cmake --build ${buildRootDir} --target compile_connectivity --parallel ${cpuCores}
    fi
}

##################################################################
# Execution
##################################################################
getCpuCores
checkAndClean
generateCmakeProject
checkAndBuild
echo "CMake Done"
