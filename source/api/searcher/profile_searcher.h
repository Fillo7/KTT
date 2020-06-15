#pragma once

#include <algorithm>
#include <random>
#include <iostream>
#include <fstream>
#include <api/searcher/searcher.h>

#define PROFILESEARCHER_TEMPFILE_CONF   "ktt-tempfile-conf.csv"
#define PROFILESEARCHER_TEMPFILE_PC     "ktt-tempfile-pc.csv"
#define PROFILESEARCHER_TEMPFILE_IDX    "ktt-tempfile-idx.dat"

namespace ktt
{

class ProfileSearcher : public Searcher
{
public:
    ProfileSearcher(const double myCC, const std::string stat, const double statCC, const std::string scratch = "") :
        Searcher(),
        myComputeCapability(myCC),
        statPrefix(stat),
        statComputeCapability(statCC),
        scratchPrefix(scratch),
        index(0)
    {}

    void onInitialize() override
    {
        // Create CSV with all configurations in the tuning space
        std::ofstream profilingFile;
        profilingFile.open (scratchPrefix + PROFILESEARCHER_TEMPFILE_CONF);
        const int pars = getConfigurations()[0].getParameterPairs().size();
        for (int i = 0; i < pars; i++) {
            profilingFile << getConfigurations()[0].getParameterPairs()[i].getName();
            if (i < pars-1) profilingFile << ",";
        }
        profilingFile << std::endl;
        for (auto conf : getConfigurations()) {
            for (int i = 0; i < pars; i++) {
                profilingFile << conf.getParameterPairs()[i].getValue();
                if (i < pars-1) profilingFile << ",";
            }
            profilingFile << std::endl;
        }
        profilingFile.close();

        // set random beginning
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, getConfigurations().size()-1);
        index = static_cast<size_t>(distribution(generator));
    }

    void onReset() override
    {
    }

    void calculateNextConfiguration(const ComputationResult& computationResult) override
    {
        std::vector<KernelProfilingCounter> counters = computationResult.getProfilingData().getAllCounters(); //getCounter("name")
        //KernelResult.getCompilationData();

        // create CSV with actual profiling counters
        std::ofstream profilingFile;
        profilingFile.open(scratchPrefix + PROFILESEARCHER_TEMPFILE_PC);

        const int cnt = counters.size();
        for (int i = 0; i < cnt; i++) {
            profilingFile << counters[i].getName();
            if (i < cnt-1) profilingFile << ",";
        }
        profilingFile << std::endl;
        for (int i = 0; i < cnt; i++) {
            switch(counters[i].getType()) {
            case ktt::ProfilingCounterType::Int:
                profilingFile << counters[i].getValue().intValue;
                break;
            case ktt::ProfilingCounterType::UnsignedInt:
                profilingFile << counters[i].getValue().uintValue;
                break;
            case ktt::ProfilingCounterType::Percent:
                profilingFile << counters[i].getValue().percentValue;
                break;
            case ktt::ProfilingCounterType::Throughput:
                profilingFile << counters[i].getValue().throughputValue;
                break;
            case ktt::ProfilingCounterType::UtilizationLevel:
                profilingFile << counters[i].getValue().utilizationLevelValue;
                break;
            case ktt::ProfilingCounterType::Double:
                profilingFile << counters[i].getValue().doubleValue;
                break;
            default:
                throw std::runtime_error("Unknown type of profiling counter.");
                break;
            }
            if (i < cnt-1) profilingFile << ",";
        }
        profilingFile.close();

        // call external python script
        std::string command = "python " + scratchPrefix + " ktt-profiling-searcher.py -o " + PROFILESEARCHER_TEMPFILE_CONF + " --oc " + std::to_string(myComputeCapability) + " --cm " + statPrefix + " --ic " + std::to_string(statComputeCapability) + " -i " + std::to_string(index) + " -p " + PROFILESEARCHER_TEMPFILE_PC;
        std::cout << command << std::endl;
        system(command.c_str());

        // read result of the script
        std::fstream indexFile(PROFILESEARCHER_TEMPFILE_IDX, std::fstream::in);
        indexFile >> index;
        indexFile.close();
        //std::cout << "Index: " << index << std::endl;
    }

    const KernelConfiguration& getNextConfiguration() const override
    {
        return getConfigurations().at(index);
    }

    size_t getUnexploredConfigurationCount() const override
    {
        if (index >= getConfigurations().size())
        {
            return 0;
        }

        return getConfigurations().size() - index;
    }

private:
    size_t index;
    double myComputeCapability;
    std::string statPrefix;
    double statComputeCapability;
    std::string scratchPrefix;
};

} // namespace ktt
