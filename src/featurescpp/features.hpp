#pragma once

//
namespace classifier {
//
// ======================= Features =======================
struct features
{
   // route travel time related
   int route_size;
   double travel_time;
   double total_delivery_time;
   double min_time_btw_stops;
   double max_time_btw_stops;
   double median_time_btw_stops;
   // package and packtrack related
   int min_packages_per_stop;
   int max_packages_per_stop;
   double std_packages_per_stop;
   int backtracking_count;
   // time window related
   double violation_total_time;
   int violation_early_count;
   double violation_early_total_time;
   double violation_early_avg_time;
   double ratio_early_violation;
   int violation_late_count;
   double violation_late_total_time;
   double violation_late_avg_time;
   double violation_avg_time;
   double ratio_late_violation;
   // rush hour related
   bool rush_hour_evening = false;
   bool rush_hour_morning = false;
   // misc ratios
   double area;
   double ratio_delivery_time_packages_count;
   double ratio_delivery_time_stops;
   double ratio_package_count_violation_avg_time;
   double ratio_package_count_violation_late_avg_time;
   double ratio_package_volume_delivery_time;
   double ratio_packages_count_delivery_time;
   double ratio_route_size_delivery_time;
   double ratio_route_size_travel_time;
   double ratio_travel_time_packages_count;
   double ratio_violation_delivery_time;
   double ratio_violation_w_total_time;
   double ratio_zones_delivery_time;
   double ratio_zones_route_size;
   double ratio_zones_travel_time;
   //
   // weekdays
   bool Friday = false;
   bool Monday = false;
   bool Saturday = false;
   bool Sunday = false;
   bool Thursday = false;
   bool Tuesday = false;
   bool Wednesday = false;
   // auxiliar
   std::pair<double, double> time_window;
   //
   friend std::ostream&
   operator<<(std::ostream& os, const features& me)
   {
      os.precision(17);
      os << "{";
      os << "\"Friday\":" << me.Friday << ",";
      os << "\"Monday\":" << me.Monday << ",";
      os << "\"Saturday\":" << me.Saturday << ",";
      os << "\"Sunday\":" << me.Sunday << ",";
      os << "\"Thursday\":" << me.Thursday << ",";
      os << "\"Tuesday\":" << me.Tuesday << ",";
      os << "\"Wednesday\":" << me.Wednesday << ",";
      os << "\"area\":" << me.area << ",";
      os << "\"backtracking_count\":" << me.backtracking_count << ",";
      os << "\"max_packages_per_stop\":" << me.max_packages_per_stop << ",";
      os << "\"max_time_btw_stops\":" << me.max_time_btw_stops << ",";
      os << "\"median_time_btw_stops\":" << me.median_time_btw_stops << ",";
      os << "\"min_packages_per_stop\":" << me.min_packages_per_stop << ",";
      os << "\"min_time_btw_stops\":" << me.min_time_btw_stops << ",";
      os << "\"ratio_delivery_time_packages_count\":" << me.ratio_delivery_time_packages_count << ",";
      os << "\"ratio_delivery_time_stops\":" << me.ratio_delivery_time_stops << ",";
      os << "\"ratio_early_violation\":" << me.ratio_early_violation << ",";
      os << "\"ratio_late_violation\":" << me.ratio_late_violation << ",";
      os << "\"ratio_package_count_violation_avg_time\":" << me.ratio_package_count_violation_avg_time << ",";
      os << "\"ratio_package_count_violation_late_avg_time\":" << me.ratio_package_count_violation_late_avg_time << ",";
      os << "\"ratio_package_volume_delivery_time\":" << me.ratio_package_volume_delivery_time << ",";
      os << "\"ratio_packages_count_delivery_time\":" << me.ratio_packages_count_delivery_time << ",";
      os << "\"ratio_route_size_delivery_time\":" << me.ratio_route_size_delivery_time << ",";
      os << "\"ratio_route_size_travel_time\":" << me.ratio_route_size_travel_time << ",";
      os << "\"ratio_travel_time_packages_count\":" << me.ratio_travel_time_packages_count << ",";
      os << "\"ratio_violation_delivery_time\":" << me.ratio_violation_delivery_time << ",";
      os << "\"ratio_violation_w_total_time\":" << me.ratio_violation_w_total_time << ",";
      os << "\"ratio_zones_delivery_time\":" << me.ratio_zones_delivery_time << ",";
      os << "\"ratio_zones_route_size\":" << me.ratio_zones_route_size << ",";
      os << "\"ratio_zones_travel_time\":" << me.ratio_zones_travel_time << ",";
      os << "\"route_size\":" << me.route_size << ",";
      os << "\"rush_hour_evening\":" << me.rush_hour_evening << ",";
      os << "\"rush_hour_morning\":" << me.rush_hour_morning << ",";
      os << "\"std_packages_per_stop\":" << me.std_packages_per_stop << ",";
      os << "\"total_delivery_time\":" << me.total_delivery_time << ",";
      os << "\"travel_time\":" << me.travel_time << ",";
      os << "\"violation_avg_time\":" << me.violation_avg_time << ",";
      os << "\"violation_early_avg_time\":" << me.violation_early_avg_time << ",";
      os << "\"violation_early_count\":" << me.violation_early_count << ",";
      os << "\"violation_early_total_time\":" << me.violation_early_total_time << ",";
      os << "\"violation_late_avg_time\":" << me.violation_late_avg_time << ",";
      os << "\"violation_late_count\":" << me.violation_late_count << ",";
      os << "\"violation_total_time\":" << me.violation_total_time << "}";
      return os;
   }
};
//
} // namespace classifier
