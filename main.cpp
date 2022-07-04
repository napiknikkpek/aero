#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <vector>

typedef char Carrier[3];   // код авиакомпании
typedef char FlightNo[5];  // номер рейса
typedef char Point[4];     // код пункта
typedef long Fare;         // тариф

// Рейс
struct Flight {
  Carrier carrier;    // перевозчик
  FlightNo flightNo;  // номер рейса
  Point depPoint;     // пункт отправления
  Point arrPoint;     // пункт назначения
  Fare fare;          // тариф
};

using My_point = std::array<char, 4>;

int generate_carrier_key(char const* carrier) {
  return (1 << 8) * carrier[0] + carrier[1];
}

int generate_point_key(char const* pt) {
  return (1 << 16) * pt[0] + (1 << 8) * pt[1] + pt[2];
}

std::uint64_t generate_point_edge_key(char const* from, char const* to) {
  int left = generate_point_key(from);
  int right = generate_point_key(to);
  return (std::uint64_t{1} << 32) * left + right;
}

void build_cheapest_transportation(auto const& schedule, auto const& route,
                                   float discount, auto out) {
  std::unordered_map<std::uint64_t, std::vector<std::size_t>> index;

  for (auto i = 0u; i < schedule.size(); ++i) {
    auto const& fl = schedule[i];
    index[generate_point_edge_key(fl.depPoint, fl.arrPoint)].push_back(i);
  }

  std::unordered_map<int, std::vector<std::size_t>> candidates;

  for (int i = 1u; i < route.size(); ++i) {
    auto it = index.find(
        generate_point_edge_key(route[i - 1].data(), route[i].data()));
    if (it == index.end()) return;

    long s = std::numeric_limits<long>::max();
    std::size_t best = 0;
    for (auto id : it->second) {
      auto const& fl = schedule[id];
      candidates[generate_carrier_key(fl.carrier)].push_back(id);
      if (fl.fare < s) {
        s = fl.fare;
        best = id;
      }
    }
    candidates[0].push_back(best);
  }

  float total = std::numeric_limits<float>::max();
  std::vector<std::size_t> const* best;

  for (auto const& [carrier, path] : candidates) {
    if (path.size() != route.size() - 1) continue;

    long s = ((carrier != 0) ? discount : 1.0) *
             std::accumulate(
                 path.begin(), path.end(), 0l,
                 [&](long s, std::size_t id) { return s + schedule[id].fare; });
    if (s < total) {
      best = &path;
      total = s;
    }
  }

  std::copy(best->begin(), best->end(), out);
}

int main() {
  std::vector<Flight> schedule;
  std::vector<My_point> route;

  FILE* f = fopen("schedule.txt", "r");

  Flight fl;
  while (fscanf(f, "%2s %4s %3s %3s %ld", fl.carrier, fl.flightNo, fl.depPoint,
                fl.arrPoint, &fl.fare) == 5) {
    schedule.push_back(fl);
  }
  std::fclose(f);

  f = fopen("route.txt", "r");

  My_point pt;
  while (fscanf(f, "%3s", pt.data()) == 1) {
    route.push_back(pt);
  }
  std::fclose(f);

  std::vector<std::size_t> result;

  build_cheapest_transportation(schedule, route, 0.8,
                                std::back_inserter(result));

  if (result.empty()) {
    std::printf("no transportation found\n");
    return 1;
  }

  for (int i = 0u; i < result.size(); ++i) {
    auto const& fl = schedule[result[i]];
    std::printf("%d: %-2s %-4s %-3s %-3s %10ld\n", i, fl.carrier, fl.flightNo,
                fl.depPoint, fl.arrPoint, fl.fare);
  }

  return 0;
}
