#ifndef EVAL_INTEGRATION_HPP
#define EVAL_INTEGRATION_HPP

#include "feature_engineering.hpp"

using FeatureEngPhase = classifier::FeatureEngineering::FeatureEngPhase;

namespace classifier {

class SolutionIntegration {
   public:

   FeatureEngineering feng;

   std::vector<std::string> listaRotas;

   std::string routeId;// = "RouteID_15baae2d-bf07-4967-956a-173d4036613f";

   classifier::features feats_de_uma_rota;

   SolutionIntegration(std::string _routeId) :
      feng{FeatureEngineering(FeatureEngPhase::PHASE_RUNTIME, "data/")}, routeId{_routeId}
   {
      listaRotas = feng.create_features(_routeId);
      std::cout << "READ: " << this->listaRotas.size() << std::endl;      
   }

   classifier::features initSequence(nlohmann::json& jsequence) {
      // first 'route_load'
      return
      feng.route_load(
         feng.package_data[routeId],
         feng.route_data[routeId],
         feng.travel_times[routeId],
         jsequence);
      
      //classifier::features& feats_de_uma_rota = feng.checkFeatures(routeId);
   }
}; // SolutionIntegration

} // namespace classifier


#endif //EVAL_INTEGRATION_HPP