#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

using namespace std;

struct Node {
  double lon, lat;
  vector<pair<Node*, double>> children;
};

struct Graph {
  vector<Node*> nodes;
  explicit Graph(const string& filename) { // Убираем неявное создание
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл\n";
        return;
    }

    map<pair<double, double>, Node*> nodeMap; // map для поиска уже созданных узлов

    string line;
    while (getline(file, line)) {
        if (line.empty())
          continue;

        // Разделим строку на первую часть (lon1,lat1) и список ребер (lon2,lat2,weight;...)
        auto delimiterPos = line.find(':');
        string nodePart = line.substr(0, delimiterPos);
        string edgesPart = line.substr(delimiterPos + 1);

        // Парсим узел (lon1, lat1)
        stringstream nodeStream(nodePart);
        double lon1, lat1;
        char comma;
        nodeStream >> lon1 >> comma >> lat1;

        // Проверяем есть ли уже узел с такими координатами
        Node* currentNode = nullptr;
        if (auto it = nodeMap.find({lon1, lat1}); it == nodeMap.end()) {
          currentNode = new Node{lon1, lat1, {}};
          nodes.push_back(currentNode);
          nodeMap[{lon1, lat1}] = currentNode;
        } else {
          currentNode = it->second;
        }

        // Парсим ребра (lon2,lat2,weight) и (lon3,lat3,weight) если есть
        stringstream edgesStream(edgesPart);
        string edge;
        while (getline(edgesStream, edge, ';')) {
          stringstream edgeStream(edge);
          double lon2, lat2, weight;

          edgeStream >> lon2 >> comma >> lat2 >> comma >> weight;

          // Проверяем есть ли узел назначения (lon2, lat2)
          Node* targetNode = nullptr;
          auto targetIt = nodeMap.find({lon2, lat2});
          if (targetIt == nodeMap.end()) {
            targetNode = new Node{lon2, lat2, {}};
            nodes.push_back(targetNode);
            nodeMap[{lon2, lat2}] = targetNode;
          } else {
            targetNode = targetIt->second;
          }

          // Добавляем ребро к текущему узлу
          currentNode->children.emplace_back(targetNode, weight); // Избегаем копирования
        }
    }

    file.close();
  }

  void freeGraph() {
    // Очистка памяти
    for (auto node : nodes) {
      delete node;
    }
    nodes.clear();
  }

  Node* findClosestNode(double lon, double lat) {
    // Функция для поиска в графе узла, который ближе всего находится к указанной точке, которую вы выбрали на карте
    double min_distance = numeric_limits<double>::infinity();
    Node* node_founded = nullptr;
    for (auto node : nodes) {
      double distance = sqrt(pow(node->lon - lon, 2) + pow(node->lat - lat, 2));
      if (distance < min_distance) {
        node_founded = node;
        min_distance = distance;
      }
    }
    return node_founded;
  }

  double dfs_x(Node* current, Node* target, unordered_map<Node*, bool>& visited, double current_distance) {
    if (current == target) {
      return current_distance; // Если нашли возвращаем расстояние
    }

    visited[current] = true; // Ставим метку что в текущем пути

    for (auto& [neighbor, weight] : current->children) {
      if (!visited[neighbor]) {
        double distance = dfs_x(neighbor, target, visited, current_distance + weight);
        if (distance != numeric_limits<double>::infinity()) // Если путь был найден то передаем длину обратно
          return distance;
      }
    }

    return numeric_limits<double>::infinity();
  }

  double dfs(Node* start, Node* target) {
    unordered_map<Node*, bool> visited;
    double result = dfs_x(start, target, visited, 0.0);
    return result == numeric_limits<double>::infinity() ? -1 : result; // Если не нашли вернем -1
  }

  double bfs(Node* start, Node* target) {
    queue<pair<Node*, int>> q;
    unordered_map<Node*, bool> visited;

    q.emplace(start, 0); // Избегаем копирования
    visited[start] = true;

    while (!q.empty()) {
      auto [current, distance] = q.front();
      q.pop();

      if (current == target) {
        return distance; // Искомое расстояние
      }

      for (auto& [neighbor, weight] : current->children) {
        if (!visited[neighbor]) {
          visited[neighbor] = true;
          q.emplace(neighbor, distance + 1); // Избегаем копирования
        }
      }
    }

    return -1; // Если не нашли расстояние вернем -1
  }

  double dijkstra(Node* start, Node* target) {
    unordered_map<Node*, double> dist; // Хранит минимальные расстояния
    // Используется минимальная куча и так как задаем пары, то нужно прописать контейнер
    priority_queue<pair<double, Node*>, vector<pair<double, Node*>>, greater<>> pq;

    dist[start] = 0.0;
    pq.emplace(0.0, start); // Избегаем копирования

    while (!pq.empty()) {
      auto [current_distance, current] = pq.top(); // Берем вершину с минимальным расстоянием
      pq.pop();

      if (current == target) {
        return current_distance; // Нашли минимальное расстояние до целевой вершины
      }

      for (auto& [neighbor, weight] : current->children) {
        double new_distance = current_distance + weight;

        if (dist.find(neighbor) == dist.end() || new_distance < dist[neighbor]) {
          dist[neighbor] = new_distance;
          pq.emplace(new_distance, neighbor);  // Избегаем копирования
        }
      }
    }

    return -1; // Если target недостижима
  }

  double a_star(Node* start, Node* target) {
    unordered_map<Node*, double> distances; // Хранит минимальные расстояния
    // Минимальная куча по функции f которая равна сумме пройденного расстояния до вершины и предположительного оставшегося расстояния
    // То есть чем меньше значение f, тем раньше откроем вершину и предположительно достигнем цели быстрее
    priority_queue<pair<double, Node*>, vector<pair<double, Node*>>, greater<>> pq;

    distances[start] = 0.0;
    pq.emplace(0.0, start); // Избегаем копирования

    while (!pq.empty()) {
      auto [current_f_score, current] = pq.top();
      pq.pop();

      if (current == target) {
        return distances[current]; // Нашли минимальное расстояние до целевой вершины
      }

      for (auto& [neighbor, weight] : current->children) {
        double new_distance = distances[current] + weight;

        if (distances.find(neighbor) == distances.end() || new_distance < distances[neighbor]) {
          distances[neighbor] = new_distance;

          // f_score = g + h
          double heuristic = haversine(neighbor->lon, neighbor->lat, target->lon, target->lat);
          double f_score = new_distance + heuristic;

          pq.emplace(f_score, neighbor); // Избегаем копирования
        }
      }
    }

    return -1; // Если target недостижима
  }

  double haversine(double lon1, double lat1, double lon2, double lat2) {
    // Формула haversine используется для вычисления кратчайшего расстояния (по дуге большого круга) между двумя точками на поверхности сферы, основываясь на их широте и долготе
    // Эвристика является допустимой так как ее значение <= реальному пути до цели
    // Эвристика является монотонной так как для любой вершины v и ее потомка u, разность их оценок <= фактического веса между ними, а также оценка целевого состояния равно 0

    const double R = 6371.0; // Радиус Земли в километрах

    // Берем разницу широты и долготы в радианах для функций
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double dlat = (lat2 - lat1) * M_PI / 180.0;

    // Дополнительная переменная
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dlon / 2) * sin(dlon / 2);
    // Угловое расстояние
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c; // Возвращаем расстояние в километрах
  }
};

int main() {
  Graph graph("../spb_graph.txt");

  double home_lon = 30.337795, home_lat = 59.926835;
  double itmo_lon = 30.295483, itmo_lat = 59.944077;

  auto home = graph.findClosestNode(home_lon, home_lat);
  auto itmo = graph.findClosestNode(itmo_lon, itmo_lat);
  cout << home->lon << " " << home->lat << '\n';
  cout << itmo->lon << " " << itmo->lat << "\n\n";

  auto start_time = chrono::high_resolution_clock::now(), end_time = chrono::high_resolution_clock::now();

  start_time = chrono::high_resolution_clock::now();
  cout << "DFS: " << graph.dfs(home, itmo) << '\n';
  end_time = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::microseconds>(end_time - start_time).count() << "\n\n";

  start_time = chrono::high_resolution_clock::now();
  cout << "BFS: " << graph.bfs(home, itmo) << '\n';
  end_time = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::microseconds>(end_time - start_time).count() << "\n\n";

  start_time = chrono::high_resolution_clock::now();
  cout << "Dijkstra: " << graph.dijkstra(home, itmo) << '\n';
  end_time = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::microseconds>(end_time - start_time).count() << "\n\n";

  start_time = chrono::high_resolution_clock::now();
  cout << "A*: " << graph.a_star(home, itmo) << '\n';
  end_time = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::microseconds>(end_time - start_time).count() << "\n\n";

  graph.freeGraph();
}
