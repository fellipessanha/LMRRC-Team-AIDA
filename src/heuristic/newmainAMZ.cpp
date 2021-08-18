
// SYSTEM LIBS
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>

// OpenMP - parallel threads!
#include <omp.h>

// must include before nlohmann::json
#include <vastjson/VastJSON.hpp>

// JSON LIBS
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// optimization components for AMZ
#include "OptAMZ.hpp"

// import everything on main()
using namespace optframe;
using namespace scannerpp;
using namespace TSP_amz;
using namespace vastjson;

#include "EvalIntegration.hpp" // from 'featurescpp'

using FeatureEngPhase = classifier::FeatureEngineering::FeatureEngPhase;

class MyEvaluatorDEBUG final
  : public Evaluator<ESolutionTSP::first_type, ESolutionTSP::second_type, ESolutionTSP>
{
   using S = ESolutionTSP::first_type;
   using XEv = ESolutionTSP::second_type;
   using XSH = ESolutionTSP; // only single objective

public:
   classifier::SolutionIntegration& global;

   MyEvaluatorDEBUG(classifier::SolutionIntegration& _global)
     : global{ _global }
   {}

   virtual Evaluation<double> evaluate(const std::vector<int>& sol)
   {
      // invoke heuristic evaluator
      Evaluation<double> e_h = ev.evaluate(sol);
      return e_h.evaluation();
   }

   virtual bool isMinimization() const { return true; }
};

int
main()
{
   srand(100001);
   Timer t_global;                    // global timer
   const double TARGET_TIME = 2 * 60; //4 * 3600 - 60; // 4 hours (- 1 minute)
   //
   std::cout << "====== BEGIN AMAZ =======" << std::endl;
   std::string dirPath = "data/model_apply_inputs/";
   std::string dirCachePath = "data/model_apply_inputs_cache/";
   //
   // =============== LOADING FILES ==============
   std::string jsonContent =
     TSP_amz::removeNaN(dirPath + "new_package_data.json");
   VastJSON new_package_data(jsonContent, BIG_ROOT_DICT_NO_ROOT_LIST);
   //
   // Being used to fetch routes.
   jsonContent = TSP_amz::removeNaN(dirPath + "new_route_data.json");
   VastJSON new_route_data(jsonContent, BIG_ROOT_DICT_NO_ROOT_LIST);
   //
   jsonContent = TSP_amz::removeNaN(dirPath + "new_travel_times.json");
   VastJSON new_travel_times(jsonContent, BIG_ROOT_DICT_NO_ROOT_LIST);
   // std::cout << "new_travel_times.size() = " << new_travel_times.size() <<
   // std::endl;
   //

   //
   // writing everything in a string because of pop_back() method for the last
   // ','
   std::string outputWriter[N_THREADS];

   // =============== SIMULATED ANNEALING ==============
   // ================ WITHOUT CLASSIFIER ==============

   std::cout << "starting new simulated annealing DEBUG!" << std::endl;
   //
   // ===================== WITH CLASSIFIER =====================
   const int PROBLEM_TOTAL = new_route_data.size();
   std::cout << "PROBLEMS = " << PROBLEM_TOTAL << std::endl;
   //
   int problem_count = 0;
   std::string route_id_vector[PROBLEM_TOTAL];
   for (auto it1 = new_route_data.begin(); it1 != new_route_data.end(); it1++) {
      std::string route_id = it1->first;
      route_id_vector[problem_count++] = route_id;
   }

   omp_set_num_threads(
     N_THREADS); // TODO: get the number of threads from somewhere
   for (int i = 0; i < N_THREADS; i++) {
      outputWriter[i] = "";
   }
   int WORKING_PROBLEMS = 0;
//#pragma omp parallel for private(problem_count) schedule(dynamic, 5)
#pragma omp parallel for private(problem_count) schedule(dynamic, 1)
   // for (int i = 0; i < 8; i++){
   for (int i = 0; i < PROBLEM_TOTAL; i++) {
      WORKING_PROBLEMS++;
      Timer t_local;
      // for (auto it1 = new_route_data.begin(); it1 != new_route_data.end();
      // it1++) { std::string route_id = it1->first;
      std::string route_id = route_id_vector[i];
      // tid = omp_get_thread_num();
      set_tid(omp_get_thread_num());
      std::cout << "ThreadID: " << tid << " " << omp_get_thread_num()
                << " routeID: " << route_id << std::endl;

      // global counter
      // global_ev_debug_count = 0;

      classifier::SolutionIntegration global{ route_id };

      pTSP[tid].load(route_id, tid, global.feng.package_data, global.feng.route_data, global.feng.travel_times);

      sref<RandGen> rg2{ new RandGen{ 2'000'000 } }; // heap version (safely shared)

      MyEvaluatorDEBUG ev_debug(global);

      BasicInitialSearch<ESolutionTSP> initRand(crand, ev);

      vsref<NS<ESolutionTSP>> neighbors;
      neighbors.push_back(nsseq2Opt);
      neighbors.push_back(nsseqSwap);
      double T = BasicSimulatedAnnealing<ESolutionTSP>::estimateInitialTemperature(
        ev_debug, initRand, neighbors, 1.1, 0.7, 500, 1, rg2);

      BasicSimulatedAnnealing<ESolutionTSP> sa(ev_debug, initRand, neighbors, 0.98, 500, T, rg2);
      sa.setSilentR();

      double G_NOW = t_global.now();

      double max_time = (TARGET_TIME - G_NOW) / ((double)PROBLEM_TOTAL - i); //WORKING_PROBLEMS);
      max_time = ::min(max_time, TARGET_TIME - G_NOW);
      if (max_time < 0)
         max_time = 0.1;
      std::cout << tid << " MAX_TIME = " << max_time << " TARGET " << TARGET_TIME << " G_NOW " << G_NOW << " PROBLEM " << PROBLEM_TOTAL << " WORKING " << i << std::endl;

      auto status = sa.search(max_time); //sa.search(30.0); // 30 seconds
      ESolutionTSP best = *status.best;
      // best solution value
      best.second.print();
      // last public evaluation
      // ev.evaluate(best.first).print();
      //global_ev_debug_count = 0;
      ev_debug.evaluate(best.first).print();

      vsref<LocalSearch<ESolutionTSP>> ns_list;
      ns_list.push_back(
        new FirstImprovement<ESolutionTSP>(ev_debug, nsseqSwap));
      ns_list.push_back(
        new FirstImprovement<ESolutionTSP>(ev_debug, nsseq2Opt));

      VariableNeighborhoodDescent<ESolutionTSP> VND(ev_debug, ns_list);
      VND.setSilentR();

      ILSLPerturbationLPlus2<ESolutionTSP> pert(ev_debug, nsseqSwap, rg2);

      IteratedLocalSearchLevels<ESolutionTSP> ils(ev_debug, initRand, VND, pert, 10, 5);
      ils.setSilentR();

      best.second = ev_debug.evaluate(best.first);
      std::cout << tid << " EVALUATION=" << best.second << std::endl;

      double max_time2 = max_time - t_local.now() - 3;
      if (max_time2 < 0)
         max_time2 = 0.1;
      std::cout << tid << " MAX_TIME2 = " << max_time << std::endl;
      // auto status = sa.search(1800.0); // 10.0 seconds max
      auto status2 = ils.searchBy(best, best, max_time2); // 10 minutes

      ESolutionTSP best2 = *status2.best;

      std::cout << tid << " EVALUATION=" << best2.second << std::endl;

      //
      std::cout << tid << " search done!" << i << std::endl;
      // should be in for loop for each iteration
      outputWriter[tid] += "\"" + route_id + "\":{\"proposed\":" +
                           pTSP[tid].returnOutput(best2.first).dump() + "},";
      // if(problem_count >= 0)
      //   break; // SINGLE ROUTE!!
      //problem_count++;
      std::cout << tid << " LOCAL TIME " << i << " " << t_local.now() << std::endl;
   }
   std::cout << "ALL THREADS ARE OUT!" << t_global.now() << std::endl;
   //
   std::string outputWriterTotal = "";
   for (int i = 0; i < N_THREADS; i++)
      outputWriterTotal += outputWriter[i];
   outputWriterTotal.pop_back(); // gets rid of last ','
   std::ofstream new_jsonOutput(
     "data/model_apply_outputs/proposed_sequences.json", std::ofstream::out);
   new_jsonOutput << "{" << outputWriterTotal << "}";
   new_jsonOutput.close();
   //
   return 0;
}
