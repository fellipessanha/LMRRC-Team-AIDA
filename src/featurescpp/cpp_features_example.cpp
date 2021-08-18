//#include "classifier.c"
#include "classifier_binario_3classes.c"

#include "feature_engineering.hpp"

#include "EvalIntegration.hpp"

using FeatureEngPhase = classifier::FeatureEngineering::FeatureEngPhase;

int
main()
{
   // O construtor tem o seguinte modelo:
   // FeatureEnginnering(fase, path de input, path de output)
   // pra a fase, true quer dizer apply, false quer dizer build
   //
   // os dados de input tem que ir só até o "data"
   // não pode pôr o model_build, etc tem um arquivo que dá ruim:
   // no model_apply, o proposed_sequences tá dentro do output, e não do input
   classifier::FeatureEngineering testinho(FeatureEngPhase::PHASE_APPLY, "data/");
   std::cout << "Montou os path tudo!\n";
   //
   // Essa função cria features pra todas as rotas da pasta
   // E armazena em um mapa de <string, features>
   // a string é o nome da rota, algo do tipo "RouteID_15baae2d-bf07-4967-956a-173d4036613f"
   // "features" no caso é o nome de uma struct que contem as features que a @dvianna me mandou usar
   // Ele também retorna uma lista de quais as rotas que foram analisadas
   std::cout << "fazendo as feature...\n";
   std::vector<std::string> listaRotas = testinho.create_features();
   std::cout << "terminou de construir!\n";
   //
   // pra acessar as features tem que fazer assim:
   std::string routeId = "RouteID_a8f0009d-e50a-49c9-84d3-f9885ad14a54";
   classifier::features& feats_de_uma_rota = testinho.checkFeatures(routeId);
   // Acessa as features individuais assim:
   std::cout << "Printando features específicos de uma rota específica:\n";
   std::cout << "packages per stop: " << feats_de_uma_rota.std_packages_per_stop << std::endl;
   std::cout << "ratio_route_size_travel_time" << feats_de_uma_rota.ratio_route_size_travel_time << std::endl;
   // Se precisar printar features de uma rota específica em algum lugar, faz assim:
   std::cout << "Todos os feats de uma rota só, em formato json, p/ a rota " << routeId << "\n";
   std::cout << feats_de_uma_rota << std::endl;
   //
   // Retornando as features de uma rota especifica como ponteiro de double:
   double* array_feats;
   array_feats = testinho.featuresAs_pointer_array(routeId);
   for (int i = 0; i < 43; i++)
      std::cout << i << "; " << array_feats[i] << "\t";
   std::cout << std::endl;
   // isso aqui outputa tudo que foi gerado em uma create_features() em um json
   std::ofstream jsonOutput("testinho.json", std::ofstream::out);
   jsonOutput << testinho;
   jsonOutput.close();
   //
   // Scoring!
   double output[2];
   score(array_feats, output);
   std::cout << "output value: " << output[1] << "; " << output[0] << std::endl;

   return 0;
}