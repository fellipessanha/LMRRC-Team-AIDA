#ifndef FEATURE_ENG_PARTIAL_HPP
#define FEATURE_ENG_PARTIAL_HPP

#ifndef FEATURE_ENG_HPP
#error "Must include 'feature_engineering.hpp' before this!"
#endif

namespace classifier
{

features
FeatureEngineering::route_load(json& package_data, json& route_data, json& travel_times_data, json& sequence_data)
   {
      //
      features rt_feats;
      int sz = sequence_data.size();
      int pkg_counter = 0;
      double pkg_volume = 0;
      std::vector<std::pair<double, double>> coordinates;
      int pkg_has_tw = 0;
      int late_pkg_has_tw = 0;
      // int tw_violated = 0;  UNUSED FOR NOW
      //
      std::set<std::string> unique_zones;
      int backtrack_counter = 0;
      //
      std::vector<std::string> vSequence(sz);
      std::vector<double> sequence_ttimes;
      //
      // features from package_data
      rt_feats.max_packages_per_stop = 0;
      rt_feats.min_packages_per_stop = 845500;
      //
      // ordering the route
      for (auto it = sequence_data.begin(); it != sequence_data.end(); it++) {
         vSequence[it.value()] = it.key();
      }
      //
      // vehicle departure time
      std::string departure_day = route_data["date_YYYY_MM_DD"];
      std::string departure_time = route_data["departure_time_utc"];
      double departure_time_utc = get_utc_time(departure_day + " " + departure_time); // to datetime
      //
      double tTime = 0;
      double dTime = 0;
      double totalTime = 0;
      int eViolationCount = 0;
      int lViolationCount = 0;
      double eViolationTime = 0;
      double lViolationTime = 0;
      double tw_total = 0;
      //
      std::string prev_zone_id = "";
      std::vector<int> pkg_in_stop;
      //
      json& rteDepot = route_data["stops"][vSequence[0]];
      coordinates.push_back(std::make_pair(rteDepot["lng"], rteDepot["lat"]));
      // loop for stops in the right order
      for (unsigned i = 1; i < vSequence.size(); i++) {
         // stopnames, will iterate on the next one. First on eis always the depo!
         std::string pstop = vSequence[i - 1];
         std::string nstop = vSequence[i];
         // referencing stop specific jsons
         json& pkgStop = package_data[nstop];
         json& rteStop = route_data["stops"][nstop];
         //
         // fetching coordinates for area feature
         coordinates.push_back(std::make_pair(rteStop["lng"], rteStop["lat"]));
         // travel time
         double travel = travel_times_data[pstop][nstop];
         tTime += travel;
         sequence_ttimes.push_back(travel);
         //
         // check for error
         if (!rteStop["zone_id"].is_null()) {
            // get zone names
            std::string nzoneid = rteStop["zone_id"];
            // check if backtrack or new zone
            if (prev_zone_id != nzoneid) {
               if (unique_zones.find(nzoneid) != unique_zones.end()) {
                  backtrack_counter++;
               } else {
                  unique_zones.insert(nzoneid);
               }
               prev_zone_id = nzoneid;
            }
         }
         //
         // package number
         //
         // iterating for packages
         int pkgNum = 0;
         bool stop_has_tw(false);
         std::pair<long double, long double> time_window;
         //
         for (json::iterator it2 = pkgStop.begin(); it2 != pkgStop.end(); ++it2) {
            pkgNum++;
            json& jpack = it2.value();
            //
            long double delivery = jpack["planned_service_time_seconds"];
            dTime += delivery;
            //
            double pkg_vol;
            double pkg_dim = jpack["dimensions"]["depth_cm"];
            pkg_vol = pkg_dim;
            pkg_dim = jpack["dimensions"]["height_cm"];
            pkg_vol *= pkg_dim;
            pkg_dim = jpack["dimensions"]["width_cm"];
            pkg_vol *= pkg_dim;
            pkg_volume += pkg_vol;
            //
            // time window related
            // double elapsed_time = departure_time_utc + dTime + tTime;
            json& jtw = jpack["time_window"];
            if (!jtw["start_time_utc"].is_null()) {
               time_window.first = get_utc_time(jtw["start_time_utc"]);
               time_window.second = get_utc_time(jtw["end_time_utc"]);
               //
               stop_has_tw = true;
            } // if has time window
         }    // iterating on package
         //
         // useful pretty much everywhere
         totalTime = dTime + tTime;
         //
         pkg_counter += pkgNum;
         pkg_in_stop.push_back(pkgNum);
         //
         // If stop has one time_window than all pkgs should enter on penalization
         if(stop_has_tw){
            // std::cout << "tw: " << time_window.second - time_window.first << std::endl;
            tw_total += time_window.second - time_window.first;
            pkg_has_tw += pkgNum;
            //
            if (departure_time_utc + totalTime <= time_window.first) {
               eViolationCount++;
               eViolationTime += time_window.first -
                                 (totalTime + departure_time_utc);
            // std::cout << time_window.first -
                        // (totalTime + departure_time_utc) << std::endl;
            }
            if (departure_time_utc + totalTime >= time_window.second) {
               lViolationCount++;
               lViolationTime += (totalTime + departure_time_utc) -
                                 time_window.second;
               late_pkg_has_tw += pkgNum;
               // std::cout << (totalTime + departure_time_utc) -
               //                   time_window.second << std::endl;
            }
         }
      }
      //
      //
      // rush hour
      long double route_endtime = departure_time_utc + totalTime;
      auto rush_time_mor =
        std::make_pair(get_utc_time(departure_day + " " + "07:00:00"),
                       get_utc_time(departure_day + " " + "09:00:00"));
      auto rush_time_eve =
        std::make_pair(get_utc_time(departure_day + " " + "17:00:00"),
                       get_utc_time(departure_day + " " + "19:00:00"));
      //
      // accounts for routes that start in a day and end in the next one
      while (departure_time_utc > rush_time_mor.second) {
         rush_time_mor.first += 24 * 3600;
         rush_time_mor.second += 24 * 3600;
      }
      while (departure_time_utc > rush_time_eve.second) {
         rush_time_eve.first += 24 * 3600;
         rush_time_eve.second += 24 * 3600;
      }
      //
      // actually check if the route goes through rush hour
      // check by the negative:
      // (routes start and end) before rush start or
      // (routes start and end) after rush end
      if (!((departure_time_utc <= rush_time_mor.first && route_endtime <= rush_time_mor.first) ||
            (departure_time_utc >= rush_time_mor.second && route_endtime >= rush_time_mor.second)))
      {
         rt_feats.rush_hour_morning = true;
      }
      if (!((departure_time_utc <= rush_time_eve.first && route_endtime <= rush_time_eve.first) ||
            (departure_time_utc >= rush_time_eve.second && route_endtime >= rush_time_eve.second)))
      {
         rt_feats.rush_hour_evening = true;
      }
      //
      int weekday = departure_time_utc / (24 * 3600);
      switch (weekday % 7) {
         case 0:
            rt_feats.Thursday = true;
            break;
         case 1:
            rt_feats.Friday = true;
            break;
         case 2:
            rt_feats.Saturday = true;
            break;
         case 3:
            rt_feats.Sunday = true;
            break;
         case 4:
            rt_feats.Monday = true;
            break;
         case 5:
            rt_feats.Tuesday = true;
            break;
         case 6:
            rt_feats.Wednesday = true;
            break;
      }
      //
      int u_zones_num = unique_zones.size();
      // route features
      rt_feats.total_delivery_time = totalTime;
      rt_feats.backtracking_count = backtrack_counter;
      rt_feats.max_packages_per_stop = *std::max_element(pkg_in_stop.begin(), pkg_in_stop.end());
      rt_feats.min_packages_per_stop = *std::min_element(pkg_in_stop.begin(), pkg_in_stop.end());
      rt_feats.std_packages_per_stop = std_dev<int>(pkg_in_stop);
      rt_feats.travel_time = tTime;
      rt_feats.route_size = sz;
      rt_feats.area = compute_area(coordinates);
      //
      // max, min, median time between stops
      // different from sz because n vertices in a line have (n-1) edges
      int n_legs = sequence_ttimes.size();
      std::sort(sequence_ttimes.begin(), sequence_ttimes.end());
      rt_feats.min_time_btw_stops = sequence_ttimes[0];
      rt_feats.max_time_btw_stops = sequence_ttimes.back();
      // it's -1 on the index because it starts from 0, not from 1
      if (n_legs % 2 == 1) {
         rt_feats.median_time_btw_stops = sequence_ttimes[n_legs / 2];
      } else {
         rt_feats.median_time_btw_stops = (sequence_ttimes[(n_legs-1) / 2] + sequence_ttimes[n_legs / 2]) / 2.0;
      }
      //
      // time window stuff
      rt_feats.violation_early_count = eViolationCount;
      rt_feats.violation_early_total_time = eViolationTime;
      rt_feats.violation_late_count = lViolationCount;
      rt_feats.violation_total_time = eViolationTime + lViolationTime;
      //
      int allViolations = eViolationCount + lViolationCount;
      // if has violation, calculate it; else, make it 0;
      if (allViolations > 0) {
         //
         rt_feats.violation_avg_time =
           rt_feats.violation_total_time / (double)allViolations;
         //
         rt_feats.ratio_package_count_violation_avg_time =
           pkg_has_tw / (eViolationTime + lViolationTime);
         //
         // if early violations
         if (eViolationCount > 0) {
            rt_feats.violation_early_avg_time =
              eViolationTime / (double)eViolationCount;
            //
            rt_feats.ratio_early_violation = eViolationTime / tw_total;
         } else {
            // no early violations
            rt_feats.violation_early_avg_time = 0;
            //
            rt_feats.ratio_early_violation = 0;
         }
         //
         // if late violations
         if (lViolationCount > 0) {
            rt_feats.violation_late_avg_time =
              lViolationTime / (double)lViolationCount;
            //
            rt_feats.ratio_package_count_violation_late_avg_time =
              late_pkg_has_tw / lViolationTime;
            //
            rt_feats.ratio_late_violation = lViolationTime / tw_total;
         } else {
            // no late violations
            rt_feats.violation_late_avg_time = 0;
            //
            rt_feats.ratio_package_count_violation_late_avg_time = 0;
            //
            rt_feats.ratio_late_violation = 0;
         }
      } else {
         // No violations at all
         rt_feats.violation_avg_time = 0;
         rt_feats.ratio_package_count_violation_avg_time = 0;
         rt_feats.violation_early_avg_time = 0;
         rt_feats.ratio_early_violation = 0;
         rt_feats.violation_late_avg_time = 0;
         rt_feats.ratio_package_count_violation_late_avg_time = 0;
         rt_feats.ratio_late_violation = 0;
      }
      //
      // ratios
      rt_feats.ratio_route_size_delivery_time = sz / totalTime;
      rt_feats.ratio_route_size_travel_time = sz / tTime;
      rt_feats.ratio_travel_time_packages_count = tTime / pkg_counter;
      rt_feats.ratio_delivery_time_packages_count = totalTime / pkg_counter;
      rt_feats.ratio_delivery_time_stops = totalTime / sz;
      rt_feats.ratio_package_volume_delivery_time = pkg_volume / totalTime;
      rt_feats.ratio_packages_count_delivery_time = pkg_counter / totalTime;
      rt_feats.ratio_violation_delivery_time = allViolations / totalTime;
      rt_feats.ratio_violation_w_total_time = (eViolationTime + lViolationTime) / tw_total;
      rt_feats.ratio_zones_delivery_time = u_zones_num / totalTime;
      rt_feats.ratio_zones_route_size = u_zones_num / (double)sz;
      rt_feats.ratio_zones_travel_time = u_zones_num / tTime;
      //
      return rt_feats;
   };


} // namespace classifier

#endif // FEATURE_ENG_PARTIAL_HPP