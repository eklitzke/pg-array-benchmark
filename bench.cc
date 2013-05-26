#include <chrono>
#include <iostream>
#include <random>
#include <sstream>

#include <boost/program_options.hpp>

#include <pqxx/pqxx>
#include <pqxx/nontransaction>

std::random_device rd;
std::default_random_engine gen(rd());

template<typename D1,
         typename D2,
         typename D3>
void InitializeDb(pqxx::nontransaction *txn,
                  std::size_t num_tags,
                  D1 &num_users_on_tag,
                  D2 &random_user,
                  D3 &split_gen,
                  double split_likelihood,
                  const bool use_or) {

  for (std::size_t i = 0; i < num_tags; i++) {
    std::vector<int> user_ids;
    int primary_user = random_user(gen);
    if (!use_or) {
      user_ids.push_back(primary_user);
    }
    if (split_gen(gen) < split_likelihood) {
      const int num_users = num_users_on_tag(gen);
      for (int i = 0; i < num_users; i++) {
        user_ids.emplace_back(random_user(gen));
      }
    }

    std::stringstream query;
    query << ("INSERT INTO tags (primary_user, user_ids) "
                            "VALUES (");
    query << primary_user << ", ARRAY[";
    if (!user_ids.empty()) {
      for (auto it = user_ids.cbegin(); it < user_ids.cend() - 1; it++) {
        query << *it << ", ";
      }
      query << user_ids.back();
    }
    query << "]::integer[])";
    txn->exec(query.str());
  }
}

template <typename D>
void DoSelects(pqxx::nontransaction *txn,
               std::size_t num_selects,
               D &random_user,
               const bool use_or) {
  for (std::size_t i = 0; i < num_selects; i++) {
    int user_id = random_user(gen);
    std::stringstream query;
    if (use_or) {
      query << "SELECT * FROM tags WHERE primary_user = "
            << user_id << " OR user_ids @> ARRAY[" << user_id << "]";
    } else {
      query << "SELECT * FROM tags WHERE user_ids @> ARRAY[" << user_id << "]";
    }
    txn->exec(query.str());
  }
}

namespace po = boost::program_options;

int main(int argc, char **argv) {
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("inserts", po::value<std::size_t>()->default_value(10000), "number of inserts")
      ("selects", po::value<std::size_t>()->default_value(10000), "number of queries to do")
      ("use-or", "do OR queries")
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  pqxx::connection conn("");
  pqxx::nontransaction txn(conn);

  const std::size_t num_tags = vm["inserts"].as<std::size_t>();
  const std::size_t num_users = num_tags / 50;
  const bool use_or = vm.count("use-or");

  std::uniform_int_distribution<int> num_users_on_tag(1, 6);
  std::uniform_int_distribution<int> rand_user(1, num_users);
  std::uniform_real_distribution<double> should_split(0.0, 1.0);

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  InitializeDb(&txn, num_tags, num_users_on_tag, rand_user, should_split, 0.1, use_or);
  end = std::chrono::system_clock::now();
  std::size_t total_micros = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();
  std::size_t micros_per_insert = total_micros / num_tags;
  std::cout << micros_per_insert << " micros per insert\n";

  std::size_t num_selects = vm["selects"].as<std::size_t>();
  start = std::chrono::system_clock::now();
  DoSelects(&txn, num_selects, rand_user, use_or);
  end = std::chrono::system_clock::now();
  total_micros = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();
  std::size_t micros_per_select = total_micros / num_tags;
  std::cout << micros_per_select << " micros per select\n";

  return 0;
}
