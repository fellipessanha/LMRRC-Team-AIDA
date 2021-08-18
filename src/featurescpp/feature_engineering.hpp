// #pragma once

#ifndef FEATURE_ENG_HPP
#define FEATURE_ENG_HPP

#include <ctime>   // <time.h> ... std::tm
#include <iomanip> // std::get_time
#include <iostream>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
//
#include <vastjson/VastJSON.hpp>
//
#include <nlohmann/json.hpp>
using json = nlohmann::json;
// keep this space!!!
#include "features.hpp"

// features_1 = ["Friday", "Monday", "Saturday", "Sunday", "Thursday", "Tuesday", "Wednesday", "area",
//               "backtracking_count", "max_packages_per_stop", "max_time_btw_stops",
//               "median_time_btw_stops", "min_packages_per_stop", "min_time_btw_stops",
//               "ratio_delivery_time_packages_count", "ratio_delivery_time_stops", "ratio_early_violation",
//               "ratio_late_violation", "ratio_package_count_violation_avg_time",
//               "ratio_package_count_violation_late_avg_time", "ratio_package_volume_delivery_time",
//               "ratio_packages_count_delivery_time", "ratio_route_size_delivery_time",
//               "ratio_route_size_travel_time", "ratio_travel_time_packages_count",
//               "ratio_violation_delivery_time",
//               "ratio_violation_w_total_time", "ratio_zones_delivery_time", "ratio_zones_route_size",
//               "ratio_zones_travel_time", "route_size", "rush_hour_evening", "rush_hour_morning",
//               "std_packages_per_stop", "total_delivery_time", "travel_time", "violation_avg_time",
//               "violation_early_avg_time", "violation_early_count", "violation_early_total_time",
//               "violation_late_avg_time", "violation_late_count", "violation_total_time"]

namespace classifier {
//

class FeatureEngineering
{
public:
   // input aux
   enum FeatureEngPhase
   {
      PHASE_BUILD = 0,
      PHASE_APPLY = 1,
      PHASE_RUNTIME = 2
   };

private:
   // output file
   // vastjson::VastJSON features_file;
   std::map<std::string, features> features_files;
   //
   //const bool phase; // true -> apply; false -> build
   const FeatureEngPhase phase;

   std::string path_input;
   std::string path_output;
   //
   std::string route_data_path;
   std::string package_data_path;
   std::string travel_time_path;
   std::string sequences_path;
   //
   // ======================= Reading Files =======================
   //
   // reads input path, outputs content in string
   // data has "NaN" instead of "null", which our json libraries can"t read
   // This fixes the problem above
   std::string removeNaN(std::string inputPath)
   {
      // loading content into string
      std::ifstream loader(inputPath);
      std::string content((std::istreambuf_iterator<char>(loader)), std::istreambuf_iterator<char>());
      loader.close();
      //
      std::regex e("(NaN)"); // matches words beginning by "sub"
      return std::regex_replace(content, e, "null");
   }
   //
   // ======================= Useful Functions =======================
   //
   // shoelace formula: https://en.wikipedia.org/wiki/Shoelace_formula
   double compute_area(std::vector<std::pair<double, double>>& points)
   {
      double dArea = 0; //(points[sz-1].first * points[0].second) - (points[sz-1].second * points[0].first);
      points.push_back(points[0]);
      int sz = points.size();
      for (int i = 1; i < sz; i++) {
         dArea += (points[i].first * points[i - 1].second) - (points[i].second * points[i - 1].first);
      }
      return fabs(dArea) / 2.0;
   }
   //
   template<class T>
   double std_dev(std::vector<T> sample)
   {
      int sz = sample.size();
      double mean = 0;
      for (int i = 0; i < sz; i++)
         mean += sample[i];
      mean /= (double)sz;
      //
      double stdev = 0;
      for (int i = 0; i < sz; i++)
         stdev += (sample[i] - mean) * (sample[i] - mean);
      return sqrt(stdev / (double)(sz - 1));
   }
   //
   long double get_utc_time(std::string s)
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
      long double tx = time_stamp;
      tx -= 3 * 3600; // GMT-3
      return tx;
   }
   //
   double featArray[43];

   //
   double* features_to_array(const features& me)
   {
      //static double featArray[43];
      int i = 0;
      //
      featArray[i] = (double)me.Friday;
      i++;
      featArray[i] = (double)me.Monday;
      i++;
      featArray[i] = (double)me.Saturday;
      i++;
      featArray[i] = (double)me.Sunday;
      i++;
      featArray[i] = (double)me.Thursday;
      i++;
      featArray[i] = (double)me.Tuesday;
      i++;
      featArray[i] = (double)me.Wednesday;
      i++;
      featArray[i] = (double)me.area;
      i++;
      //
      featArray[i] = (double)me.backtracking_count;
      i++;
      featArray[i] = (double)me.max_packages_per_stop;
      i++;
      featArray[i] = (double)me.max_time_btw_stops;
      i++;
      featArray[i] = (double)me.median_time_btw_stops;
      i++;
      featArray[i] = (double)me.min_packages_per_stop;
      i++;
      featArray[i] = (double)me.min_time_btw_stops;
      i++;
      featArray[i] = (double)me.ratio_delivery_time_packages_count;
      i++;
      //
      featArray[i] = (double)me.ratio_delivery_time_stops;
      i++;
      featArray[i] = (double)me.ratio_early_violation;
      i++;
      featArray[i] = (double)me.ratio_late_violation;
      i++;
      featArray[i] = (double)me.ratio_package_count_violation_avg_time;
      i++;
      featArray[i] = (double)me.ratio_package_count_violation_late_avg_time;
      i++;
      featArray[i] = (double)me.ratio_package_volume_delivery_time;
      i++;
      featArray[i] = (double)me.ratio_packages_count_delivery_time;
      i++;
      //
      featArray[i] = (double)me.ratio_route_size_delivery_time;
      i++;
      featArray[i] = (double)me.ratio_route_size_travel_time;
      i++;
      featArray[i] = (double)me.ratio_travel_time_packages_count;
      i++;
      featArray[i] = (double)me.ratio_violation_delivery_time;
      i++;
      featArray[i] = (double)me.ratio_violation_w_total_time;
      i++;
      featArray[i] = (double)me.ratio_zones_delivery_time;
      i++;
      featArray[i] = (double)me.ratio_zones_route_size;
      i++;
      //
      featArray[i] = (double)me.ratio_zones_travel_time;
      i++;
      featArray[i] = (double)me.route_size;
      i++;
      featArray[i] = (double)me.rush_hour_evening;
      i++;
      featArray[i] = (double)me.rush_hour_morning;
      i++;
      featArray[i] = (double)me.std_packages_per_stop;
      i++;
      featArray[i] = (double)me.total_delivery_time;
      i++;
      featArray[i] = (double)me.travel_time;
      i++;
      //
      featArray[i] = (double)me.violation_avg_time;
      i++;
      featArray[i] = (double)me.violation_early_avg_time;
      i++;
      featArray[i] = (double)me.violation_early_count;
      i++;
      featArray[i] = (double)me.violation_early_total_time;
      i++;
      featArray[i] = (double)me.violation_late_avg_time;
      i++;
      featArray[i] = (double)me.violation_late_count;
      i++;
      featArray[i] = (double)me.violation_total_time;
      i++;
      return featArray;
   }

   //
public:
   //
   // ======================= Construtor de features =======================
   features
   route_load(json& package_data, json& route_data, json& travel_times_data, json& sequence_data);
   // TODO: this is implemented in partial class!
   //
   double* featuresAs_pointer_array(std::string routeId)
   {
      features feats = features_files[routeId];
      return features_to_array(feats);
   }

   double* featuresAs_pointer_array(features& feats)
   {
      return features_to_array(feats);
   }

   //
   vastjson::VastJSON actual_sequences;
   vastjson::VastJSON package_data;
   vastjson::VastJSON route_data;
   vastjson::VastJSON travel_times;

   //
   std::vector<std::string> create_features(std::string _route_id = "")
   {
      std::cout << "carregando as coisa\n";
      // std::ifstream sequencia ("data/model_apply_inputs/new_actual_sequences.json");
      // std::cout << sequencia.rdbuf() << "\n";

      //
      this->actual_sequences.clear();
      //
      if (phase != PHASE_RUNTIME) {
         std::cout << "WILL LOAD actual sequences" << std::endl;
         this->actual_sequences = vastjson::VastJSON(
           new std::ifstream(path_input + sequences_path),
           vastjson::BIG_ROOT_DICT_NO_ROOT_LIST);
         std::cout << path_input + sequences_path << "; " << actual_sequences.size() << "\n";
      }
      //
      // must replace "NaN" to "null"
      std::string jsonContent = removeNaN(path_input + package_data_path);
      this->package_data = vastjson::VastJSON(jsonContent, vastjson::BIG_ROOT_DICT_NO_ROOT_LIST);
      std::cout << path_input + package_data_path << "; " << package_data.size() << "\n";
      //
      // Being used to fetch routes.
      // must replace "NaN" to "null"
      jsonContent = removeNaN(path_input + route_data_path);
      this->route_data = vastjson::VastJSON(jsonContent,
                                            vastjson::BIG_ROOT_DICT_NO_ROOT_LIST);
      std::cout << path_input + route_data_path << "; " << route_data.size() << "\n";
      //
      // VastJSON specific gambiarra:
      // load through ifstream so it's lighter
      this->travel_times = vastjson::VastJSON(
        new std::ifstream(path_input + travel_time_path),
        vastjson::BIG_ROOT_DICT_NO_ROOT_LIST);
      std::cout << path_input + travel_time_path << "\n";
      //
      std::vector<std::string> routesList;

      //auto it = package_data.begin();
      for (auto it = package_data.begin(); it != package_data.end(); ++it) {
         std::string route_id = it->first; // default
         if (_route_id != "")              // default
                                           // for (auto it = package_data.begin(); it != package_data.end(); ++it) {
            route_id = _route_id;          // override
         //
         routesList.push_back(route_id);
         std::cout << "Fazendo as features da rota '" << route_id << "'\n";
         features routeFeats;

         if (sequences_path != "") {
            if (phase == PHASE_APPLY) {
               routeFeats =
                 route_load(
                   package_data[route_id],
                   route_data[route_id],
                   travel_times[route_id],
                   actual_sequences[route_id]["proposed"]);
            } else if (phase == PHASE_BUILD) {
               routeFeats =
                 route_load(
                   package_data[route_id],
                   route_data[route_id],
                   travel_times[route_id],
                   actual_sequences[route_id]["actual"]);
            } else {

               exit(1);
            }
         } else if (phase == PHASE_RUNTIME) {
            return routesList;
         } else {
            exit(1);
         }
         std::cout << "foi!\n";
         //
         features_files.insert({ route_id, routeFeats });
         if (_route_id != "")
            break; // FINISH LOOP
      }            // end for loop
      return routesList;
   }
   //
   friend std::ostream& operator<<(std::ostream& os, const FeatureEngineering& fe)
   {
      os.precision(17);
      os << "{";
      int counter = 0;
      int sz = fe.features_files.size();
      for (auto i = fe.features_files.begin(); i != fe.features_files.end(); i++) {
         counter++;
         os << "\"" << i->first + "\":" << i->second;
         if (counter < sz)
            os << ",";
      }
      os << "}";
      return os;
   }
   //
   features& checkFeatures(std::string routeId)
   {
      return features_files[routeId];
   }
   //
   FeatureEngineering(const FeatureEngPhase _phase = PHASE_BUILD, const std::string _path_input = ".", const std::string _path_output = ".")
     : phase{ _phase }
     , path_input{ _path_input }
     , path_output{ _path_output }
   {
      if (phase == PHASE_APPLY) {
         route_data_path = "model_apply_inputs/new_route_data.json";
         package_data_path = "model_apply_inputs/new_package_data.json";
         travel_time_path = "model_apply_inputs/new_travel_times.json";
         sequences_path = "model_apply_outputs/proposed_sequences.json";
         // data/model_apply_inputs/new_travel_times.json
      } else if (phase == PHASE_BUILD) {
         route_data_path = "model_build_inputs_100/route_data.json";
         package_data_path = "model_build_inputs_100/package_data.json";
         travel_time_path = "model_build_inputs_100/travel_times.json";
         sequences_path = "model_build_inputs_100/actual_sequences.json";
         // data/model_build_inputs_100/travel_times.json
      } else if (phase == PHASE_RUNTIME) {

         route_data_path = "model_apply_inputs/new_route_data.json";
         package_data_path = "model_apply_inputs/new_package_data.json";
         travel_time_path = "model_apply_inputs/new_travel_times.json";
         sequences_path = "";

         /*
         route_data_path = "model_apply_inputs_cache/new_route_data.json";
         package_data_path = "model_apply_inputs_cache/new_package_data.json";
         travel_time_path = "model_apply_inputs_cache/new_travel_times.json";
         sequences_path = "";
         */
      } else {
         std::cerr << "NO PHASE!" << std::endl;
         exit(1);
      }
   }

}; // class
//
} // namespace

// ============== DO NOT DO THIS (UNLESS NECESSARY!!) ===============
#include "feature_engineering_partial.hpp"

#endif //FEATURE_ENG_HPP
