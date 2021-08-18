#pragma once

#define N_THREADS 4

#include <algorithm>
#include <ctime> // <time.h> ... std::tm
#include <functional>
#include <iomanip> // std::get_time
#include <iostream>
#include <regex>
#include <set>

#include <OptFCore/FCore.hpp>
#include <OptFrame/Core.hpp>
#include <OptFrame/Heuristics/Heuristics.hpp> // many metaheuristics here...
#include <OptFrame/Scanner++/Scanner.hpp>
#include <OptFrame/Util/Matrix.hpp>
#include <OptFrame/printable/printable.h>

#include <OptFrame/Heuristics/LocalSearches/BestImprovement.hpp>
#include <OptFrame/Heuristics/LocalSearches/FirstImprovement.hpp>
#include <OptFrame/Heuristics/LocalSearches/VariableNeighborhoodDescent.hpp>

#include <OptFrame/Heuristics/ILS/ILSLPerturbation.hpp>
#include <OptFrame/Heuristics/ILS/IteratedLocalSearchLevels.hpp>

// JSON LIBS
#include <nlohmann/json.hpp>
#include <vastjson/VastJSON.hpp>
using json = nlohmann::json;

using namespace vastjson;

using namespace optframe;
using namespace scannerpp;

// next definitions come here within namespace
// this also works when defining in global scope (same as 'class')
namespace TSP_amz {

// data has "NaN" instead of "null", which our json libraries can't read
// This fixes the problem above
std::string
removeNaN(std::string inputPath, std::string toReplace = "NaN", std::string replacer = "null")
{
   // loading content into string
   std::ifstream loader(inputPath);
   std::string content((std::istreambuf_iterator<char>(loader)), std::istreambuf_iterator<char>());
   loader.close();
   //
   // replace all 'NaN' to 'null'  VastJSON new_package_data(
   std::regex e("(" + toReplace + ")"); // matches words beginning by "sub"

   return std::regex_replace(content, e, replacer);
}

// define TSP solution type as 'vector<int>', using 'double' as evaluation type
using ESolutionTSP = std::pair<
  std::vector<int>,  // first part of search space element: solution (representation)
  Evaluation<double> // second part of search space element: evaluation (objective value)
  >;

struct PackageAMZ
{
   std::string package_id; // ex: "PackageID_07017709-2ddd-4c6a-8b7e-ebde70a4f0fa": {
   // "time_window"
   // May be NaN, has to check that before with json.is_null() method
   std::string start_time_utc;
   std::string end_time_utc;
   //
   long l_start_time_utc;
   long l_end_time_utc;
   //
   double planned_service_time_seconds; // in seconds
   //
   // "dimensions"
   double depth_cm;
   double height_cm;
   double width_cm;
   double dimension; // product of the 3 dimensions, the 'volume'
};

struct StopAMZ
{
   std::string stop_id;
   vector<PackageAMZ> packages;

   // ================
   // from route_data
   std::string zone_id;
   double lat;
   double lng;
   // infered from "type" variable in each stop
   bool station; // 'false' if 'dropoff'

   // ========== summary =========
   // time window ???
   // dimension ???
};

struct RouteDataAMZ
{
   std::string station_code;       // ex: "DCH4",
   std::string date_YYYY_MM_DD;    // ex: "2018-08-11"
   std::string departure_time_utc; // ex: "15:12:44"
   long l_departure_datetime_utc;
   long executor_capacity_cm3; // ex: 4247527; must be smaller than sum of capacities
   int station_id;             // order in the solution output, i.e. 5 means will be the 5th visited point
};

// TSP problem context and data reads
class ProblemContext
{
public:
   int n; // number of stops
   vector<StopAMZ> stops;
   map<std::string, int> rev_stop_id;
   //
   json jdist; // COPY
   double minimum_total_time;
   //
   RouteDataAMZ rdata;
   //
   long max_tw()
   {
      return 2'000'000'000;
   }
   //
   long min_tw()
   {
      return 0;
   }
   //
   long get_utc_time(std::string s)
   {
      std::tm t{};
      std::istringstream ss(s);
      // 2018-08-11 11:00:00Z
      ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
      if (ss.fail()) {
         throw std::runtime_error{ "failed to parse time string" };
      }
      std::time_t time_stamp = mktime(&t);
      // may be consistently wrong by 3 hours
      // https://stackoverflow.com/questions/56884307/read-time-from-string-as-gmt-in-c
      // %Z ...
      long tx = time_stamp;
      tx -= 3 * 3600; // GMT-3
      return tx;
   }

   void clear()
   {
      stops.clear();
      rev_stop_id.clear();
      jdist.clear();
   }

   //std::vector<int> getRoute(std::string route_id, nlohmann::json& actual_sequences) {
   std::vector<int> getRoute(std::string route_id, VastJSON& actual_sequences)
   {
      nlohmann::json& jroute = actual_sequences[route_id];
      //std::cout << "jroute.size() = " << jroute.size() << std::endl;
      nlohmann::json& actual = actual_sequences[route_id]["actual"];
      //std::cout << actual << std::endl;
      std::cout << actual.size() << std::endl;

      std::vector<std::string> vstops(this->n);
      for (auto& jstop : actual.items()) {
         std::string stop_name = jstop.key();
         int stop_pos = jstop.value();
         vstops.at(stop_pos) = stop_name;
      }
      ///std::cout << vstops << std::endl;

      std::vector<int> v(this->n);

      // {
      //   AA:  1,
      //   BB : 0,
      //   CC : 2
      // }
      // pTSP.stops : [AA, BB, CC]
      // rev_stop_id[AA] = 0
      // rev_stop_id[BB] = 1
      // rev_stop_id[CC] = 2
      // -> [BB, AA, CC]
      // [1, 0, 2]
      //

      for (int i = 0; i < this->n; i++)
         //v[rev_stop_id[vstops[i]]] = i;
         v[i] = rev_stop_id[vstops[i]];
      //std::cout << v << std::endl;
      //

      // ======================================
      // double check (TODO: REMOVE!)
      // ======================================
      std::vector<std::string> vstops2(this->n);
      for (int i = 0; i < this->n; i++)
         vstops2[i] = this->stops[v[i]].stop_id;
      //std::cout << vstops2 << std::endl;
      assert(vstops == vstops2);
      // ======================================

      return v;
   }
   //
   // gives an inferior quota for the travel_time
   double traveltimequota(json& route_times)
   {
      double routemin = 0;
      for (json::iterator it1 = route_times.begin(); it1 != route_times.end(); ++it1) {
         std::string stop = it1.key();
         double minvalue = 86400;
         json& jstop = route_times[stop];
         for (json::iterator it2 = jstop.begin(); it2 != jstop.end(); ++it2) {
            if (stop != it2.key() && it2.value() < minvalue)
               minvalue = it2.value();
         }
         routemin += minvalue;
      }
      return routemin;
   }
   //
   // load data from Scanner
   void load(std::string route_id, int tid, VastJSON& new_package_data, VastJSON& new_route_data, VastJSON& new_travel_times)
   {
      //std::cout << "BEGIN load '" << route_id << "'" << "TID: " << tid << std::endl;
      this->clear();
      //
      json& jpackage = new_package_data[route_id];
      //
      json& jroutedata = new_route_data[route_id];
      json& jroutedata_stops = jroutedata["stops"];
      rdata.station_code = jroutedata["station_code"];
      rdata.date_YYYY_MM_DD = jroutedata["date_YYYY_MM_DD"];
      rdata.departure_time_utc = jroutedata["departure_time_utc"];

      std::stringstream ssdatetime;
      ssdatetime << rdata.date_YYYY_MM_DD << " " << rdata.departure_time_utc;
      rdata.l_departure_datetime_utc = get_utc_time(ssdatetime.str()); // to datetime
      //std::cout << rdata.l_departure_datetime_utc << std::endl;
      //std::cout << rdata.date_YYYY_MM_DD << std::endl;
      //std::cout << rdata.departure_time_utc << std::endl;

      rdata.executor_capacity_cm3 = jroutedata["executor_capacity_cm3"];
      // COPY
      this->jdist = new_travel_times[route_id];
      // inferior quota for travel time
      minimum_total_time = traveltimequota(jdist);
      //
      int count = 0;

      for (json::iterator it = jpackage.begin(); it != jpackage.end(); ++it) {
         //std::cout << it.key() << std::endl;
         std::string stopKey = it.key();
         rev_stop_id[stopKey] = count++;

         StopAMZ stop;
         stop.stop_id = stopKey;

         json& jstop = it.value();
         // look for packages inside stop
         for (json::iterator it2 = jstop.begin(); it2 != jstop.end(); ++it2) {
            json& jpack = it2.value();
            PackageAMZ pkg;

            pkg.package_id = it2.key();
            //
            json& jtw = jpack["time_window"];
            //
            //std::cout << pkg.package_id << std::endl;
            //
            pkg.start_time_utc = "";
            pkg.l_start_time_utc = min_tw();
            if (!jtw["start_time_utc"].is_null()) {
               pkg.start_time_utc = jtw["start_time_utc"];
               pkg.l_start_time_utc = get_utc_time(pkg.start_time_utc);
            }
            pkg.end_time_utc = "";
            pkg.l_end_time_utc = max_tw();
            if (!jtw["end_time_utc"].is_null()) {
               pkg.end_time_utc = jtw["end_time_utc"];
               pkg.l_end_time_utc = get_utc_time(pkg.end_time_utc);
            }
            //
            //std::cout << pkg.package_id << std::endl;
            //std::cout << "time: '" << pkg.start_time_utc << " " << pkg.l_start_time_utc <<  std::endl;
            //std::cout << "time: '" << pkg.end_time_utc << " " << pkg.l_end_time_utc <<  std::endl;
            //
            pkg.planned_service_time_seconds = jpack["planned_service_time_seconds"];
            // every package must be delivered, so it enters the quota
            minimum_total_time += pkg.planned_service_time_seconds;
            //
            pkg.depth_cm = jpack["dimensions"]["depth_cm"];
            pkg.height_cm = jpack["dimensions"]["height_cm"];
            pkg.width_cm = jpack["dimensions"]["width_cm"];
            pkg.dimension = pkg.depth_cm * pkg.height_cm * pkg.width_cm;
            //
            stop.packages.push_back(pkg);
         }
         // ========= new_route_data
         json& jstop2 = jroutedata_stops[stopKey];
         stop.lat = jstop2["lat"];
         stop.lng = jstop2["lng"];
         //
         if (!jstop2["zone_id"].is_null())
            stop.zone_id = jstop2["zone_id"];
         stop.station = (jstop2["type"] == "Station");
         //
         stops.push_back(stop);
         //
         if (stop.station) {
            rdata.station_id = ((int)stops.size()) - 1;
            //std::cout << "STATION = " << stopKey << " ID = " << rdata.station_id << std::endl;
         }
      }

      // 'n' comes from packages...
      this->n = stops.size();
      assert((unsigned)count == stops.size());

      //std::cout << "stops:" << n << std::endl;

      //std::cout << "END load" << std::endl;
   }

   json returnOutput(const std::vector<int>& queue)
   {
      assert(queue.size() == rev_stop_id.size());
      map<std::string, int> ret = this->rev_stop_id;
      vector<std::string> stopnames;

      for (std::map<std::string, int>::iterator it = ret.begin(); it != ret.end(); ++it) {
         stopnames.push_back(it->first);
      }

      for (auto i = 0; i < (int)queue.size(); i++) {
         ret[stopnames[queue[i]]] = i;
      }

      return json(ret);
   }
};
//

// Create TSP Problem Context
//std::vector<ProblemContext> pTSP;
ProblemContext pTSP[N_THREADS];
int tid = -1;
// Essential directive to become tid private for the omp threads
#pragma omp threadprivate(tid)

struct EvalAMZ
{
   double e_before_tw;  // seconds
   int count_before_tw; // # events
   double e_after_tw;   // seconds
   int count_after_tw;  // # events
   double e_dist;       // seconds
   std::set<string> unique_zones;
   int backtrack_counter; // # events

   void print()
   {
      std::cout << "{dist=" << e_dist
                << "; before_tw=" << e_before_tw << " #=" << count_before_tw
                << "; after_tw=" << e_after_tw << " #=" << count_after_tw << "}";
   }
};

void
set_tid(int id)
{
   tid = id;
}

EvalAMZ
fevaluate_amz(const std::vector<int>& s)
{
   EvalAMZ f_amz;
   //
   f_amz.e_before_tw = 0.0;
   f_amz.e_after_tw = 0.0;
   f_amz.e_dist = 0.0;
   f_amz.count_before_tw = 0;
   f_amz.count_after_tw = 0;
   f_amz.backtrack_counter = 0;
   f_amz.unique_zones.clear();

   //
   assert(s.size() == (unsigned)pTSP[tid].n);
   //
   assert(s[0] == pTSP[tid].rdata.station_id);
   //
   // TODO: consider "zone backtracking" (?)
   // TODO: consider "vehicle accumulated load" (?) - "green vrp"
   // consider "time windows"
   // load initial time in seconds
   double utc_route = pTSP[tid].rdata.l_departure_datetime_utc;
   //
   for (int i = 0; i < ((int)s.size()) - 1; i++) {
      //
      // advance route (in distance/time)
      double d = pTSP[tid].jdist[pTSP[tid].stops[s[i]].stop_id][pTSP[tid].stops[s[i + 1]].stop_id];
      f_amz.e_dist += d;
      // update route to i+1
      utc_route += d;
      //
      // checks backtracking
      if (pTSP[tid].stops[s[i]].zone_id != pTSP[tid].stops[s[i + 1]].zone_id) {
         f_amz.unique_zones.insert(pTSP[tid].stops[s[i]].zone_id);
         f_amz.backtrack_counter++;
      }
      //
      // check time windows (for each package)
      for (unsigned p = 0; p < pTSP[tid].stops[s[i + 1]].packages.size(); p++) {
         PackageAMZ& pkg = pTSP[tid].stops[s[i + 1]].packages[p];
         if (utc_route < pkg.l_start_time_utc) {
            f_amz.e_before_tw += ::fabs(pkg.l_start_time_utc - utc_route);
            f_amz.count_before_tw++;
         }
         if (utc_route > pkg.l_end_time_utc) {
            f_amz.e_after_tw += ::fabs(utc_route - pkg.l_end_time_utc);
            f_amz.count_after_tw++;
         }
      }
      //
      // update planned service times (on i+1)
      for (unsigned p = 0; p < pTSP[tid].stops[s[i + 1]].packages.size(); p++) {
         PackageAMZ& pkg = pTSP[tid].stops[s[i + 1]].packages[p];
         utc_route += pkg.planned_service_time_seconds;
         f_amz.e_dist += pkg.planned_service_time_seconds;
      }
   }
   //
   f_amz.e_dist -= pTSP[tid].minimum_total_time;
   f_amz.backtrack_counter -= f_amz.unique_zones.size();
   return f_amz;
}

Evaluation<double>
fevaluate(const std::vector<int>& s)
{
   double f = 0;
   constexpr double W_BEFORE_TW = 10;       // TODO: adjust
   constexpr double W_AFTER_TW = 86400;     //100000;  // TODO: adjust
   constexpr double W0_BEFORE_TW = 0;       //10;    // TODO: adjust
   constexpr double W0_AFTER_TW = 0;        //100000; // TODO: adjust
   constexpr double W_BACKTRACKING = 86400; //100000;
   //
   EvalAMZ f_amz = fevaluate_amz(s);
   //
   f += f_amz.e_dist * 1;
   f += f_amz.e_before_tw * W_BEFORE_TW + f_amz.count_before_tw * W0_BEFORE_TW;
   f += f_amz.e_after_tw * W_AFTER_TW + f_amz.count_after_tw * W0_AFTER_TW;
   f += f_amz.backtrack_counter * W_BACKTRACKING;

   return Evaluation<double>{ f };
}

// Evaluate
FEvaluator<ESolutionTSP, MinOrMax::MINIMIZE>
  ev{
     fevaluate
  };

// ===========================

std::vector<int>
frandom()
{
   vector<int> v(pTSP[tid].n, -1); // get information from context
   for (int i = 0; i < (int)v.size(); i++) {
      if (i != pTSP[tid].rdata.station_id)
         v[i] = i;
      else {
         v[i] = v[0];
         v[0] = i;
      }
   }
   // shuffle after first position (station_id is preserved in first position)
   std::random_shuffle(v.begin() + 1, v.end());
   return v;
}

// Generate random solution
FConstructive<std::vector<int>> crand{
   frandom
};

// Move Template
using MoveTemplate = FMove<std::pair<int, int>, ESolutionTSP>;

std::pair<int, int>
fApplySwap(const std::pair<int, int>& moveData, ESolutionTSP& se)
{
   int i = moveData.first;
   int j = moveData.second;
   //std::cout << "i=" << i << " j=" << j << " sz=" << se.first.size() << std::endl;
   // perform swap of clients i and j
   int aux = se.first[j];
   se.first[j] = se.first[i];
   se.first[i] = aux;
   return std::pair<int, int>(j, i); // return a reverse move ('undo' move)s
}

std::pair<int, int>
fApply2Opt(const std::pair<int, int>& moveData, ESolutionTSP& se)
{
   int i = moveData.first;
   int j = moveData.second;
   int len = (j - i) / 2;
   // perform the 2-opt
   for (int x = 0; x <= len; x++) {
      int aux = se.first[j - x];
      se.first[j - x] = se.first[i + x];
      se.first[i + x] = aux;
   }
   return std::pair<int, int>(i, j); // return a reverse move ('undo' move)s
}

// Swap move (NSSeq) - with "Boring" iterator
//
sref<NSSeq<ESolutionTSP>> nsseqSwap{
   new FNSSeq<std::pair<int, int>, ESolutionTSP>{
     [](const ESolutionTSP& se) -> uptr<Move<ESolutionTSP>> {
        int i = 1 + (rand() % (pTSP[tid].n - 2));
        // 1 <= j < pTSP.n - i; j is random
        int j = i + 1 + (rand() % (pTSP[tid].n - i - 1)); // TODO: check!
        assert(i < j);
        assert(j < se.first.size());
        //std::cout << "random: j=" << j << " sz=" << se.first.size() << std::endl;
        return uptr<Move<ESolutionTSP>>(new MoveTemplate{ make_pair(i, j), fApplySwap });
     },
     // iterator initialization (fGenerator)
     [](const ESolutionTSP& se) -> std::pair<int, int> {
        return make_pair(-1, -1);
     },
     [](std::pair<int, int>& p) -> void {
        //void (*fFirst)(IMS&),                   // iterator.first()
        p.first = 1;
        p.second = 2;
     },
     [](std::pair<int, int>& p) -> void {
        //void (*fNext)(IMS&),                    // iterator.next()
        if (p.second < (pTSP[tid].n - 1))
           p.second++;
        else {
           p.first++;
           p.second = p.first + 1;
        }
     },
     [](std::pair<int, int>& p) -> bool {
        //bool (*fIsDone)(IMS&),                  // iterator.isDone()
        return p.first >= pTSP[tid].n - 1;
     },
     [](std::pair<int, int>& p) -> uptr<Move<ESolutionTSP>> {
        //uptr<Move<XES>> (*fCurrent)(IMS&)       // iterator.current()
        return uptr<Move<ESolutionTSP>>(new MoveTemplate{ p, fApplySwap });
     } } // FNSSeq
};       // nsseqSwap

sref<NSSeq<ESolutionTSP>> nsseq2Opt{
   new FNSSeq<std::pair<int, int>, ESolutionTSP>{
     [](const ESolutionTSP& se) -> uptr<Move<ESolutionTSP>> {
        // i HAS to be smaller than j
        int i = (rand() % (pTSP[tid].n - 4)) + 1;
        // 1 <+ j < pTSP.n - i; j is random
        int j = (rand() % (pTSP[tid].n - 4)) + 1;
        while (i == j)
           j = (rand() % (pTSP[tid].n - 4)) + 1;
        return uptr<Move<ESolutionTSP>>(new MoveTemplate{ make_pair(i, j), fApply2Opt });
     },
     // iterator initialization (fGenerator)
     [](const ESolutionTSP& se) -> std::pair<int, int> {
        return make_pair(-1, -1);
     },
     [](std::pair<int, int>& p) -> void {
        //void (*fFirst)(IMS&),                   // iterator.first()
        p.first = 1;
        p.second = 2;
     },
     [](std::pair<int, int>& p) -> void {
        //void (*fNext)(IMS&),                    // iterator.next()
        if (p.second < (pTSP[tid].n - 1))
           p.second++;
        else {
           p.first++;
           p.second = p.first + 1;
        }
     },
     [](std::pair<int, int>& p) -> bool {
        //bool (*fIsDone)(IMS&),                  // iterator.isDone()
        return p.first >= pTSP[tid].n - 1;
     },
     [](std::pair<int, int>& p) -> uptr<Move<ESolutionTSP>> {
        //uptr<Move<XES>> (*fCurrent)(IMS&)       // iterator.current()
        return uptr<Move<ESolutionTSP>>(new MoveTemplate{ p, fApply2Opt });
     } } // FNSSeq
};       // nsseq2Opt

} // TSP_fcore
