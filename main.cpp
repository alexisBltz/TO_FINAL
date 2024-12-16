#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <json/json.h>
#include <chrono>
#include <mutex>
#include <atomic>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

std::mutex file_mutex;
std::atomic<bool> file_updated(false);  // Flag para indicar si el archivo CSV fue actualizado

bool is_risky(double promedio, double maximo) {
    return (promedio > 50 || maximo > 200);
}

// Función principal de procesamiento de datos usando Hadoop
Json::Value process_data_with_hadoop(const std::vector<std::string>& data) {
    // Guardar los datos en un archivo temporal
    std::ofstream temp_file("input.txt");
    for (const auto& record : data) {
        temp_file << record << "\n";
    }
    temp_file.close();

    // Subir archivo a HDFS
    int ret_code = system("hdfs dfs -put -f input.txt /user/your_user/input.txt");
    if (ret_code != 0) {
        throw std::runtime_error("Error al subir archivo a HDFS");
    }

    // Ejecutar un job de Hadoop
    ret_code = system("hadoop jar MyJob.jar MyJobClass /user/your_user/input.txt /user/your_user/output_dir");
    if (ret_code != 0) {
        throw std::runtime_error("Error al ejecutar el job de Hadoop");
    }

    // Descargar resultados desde HDFS
    ret_code = system("hdfs dfs -get -f /user/your_user/output_dir/part-r-00000 output.txt");
    if (ret_code != 0) {
        throw std::runtime_error("Error al descargar el resultado de Hadoop");
    }

    // Leer el a[rchivo de salida generado por Hadoop
    std::ifstream output_file("output.txt");
    if (!output_file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de salida de Hadoop");
    }

    double sum = 0, max_val = -1e9, min_val = 1e9;
    int count = 0;
    std::string line;
    while (std::getline(output_file, line)) {
        double value = std::stod(line); // Suponiendo que cada línea contiene un número
        sum += value;
        max_val = std::max(max_val, value);
        min_val = std::min(min_val, value);
        ++count;
    }
    output_file.close();

    // Limpiar archivos temporales
    std::remove("input.txt");
    std::remove("output.txt");

    // Construir la respuesta en formato JSON
    Json::Value result;
    result["Promedio"] = sum / count;
    result["Máximo"] = max_val;
    result["Mínimo"] = min_val;
    return result;
}

std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

void websocket_session(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        std::cout << "WebSocket conectado.\n";

        static std::streampos last_pos = 0;  // Posición del archivo para evitar leer registros anteriores
        bool first_line = true;  // Bandera para saltar la primera línea de encabezado

        while (true) {
            std::lock_guard<std::mutex> lock(file_mutex);

            // Si el archivo no ha sido actualizado, esperar un poco
            if (!file_updated.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            std::ifstream file("aire_datos.csv", std::ios::in);
            if (!file.is_open()) {
                std::cerr << "No se pudo abrir el archivo CSV\n";
                break;
            }

            file.seekg(last_pos);  // Asegura que se lea a partir de la última posición

            std::string line;
            bool data_found = false;  // Bandera para verificar si se encontró nuevos datos

            while (std::getline(file, line)) {
                line = trim(line);

                if (line.empty()) {
                    continue;
                }

                // Ignorar la primera línea del encabezado
                if (first_line) {
                    first_line = false;  // Después de la primera línea, procesamos las siguientes
                    last_pos = file.tellg();  // Actualizamos la posición
                    continue;  // Pasamos a la siguiente línea
                }

                std::istringstream ss(line);
                std::string fecha, promedio_str, maximo_str, minimo_str;

                try {
                    std::getline(ss, fecha, ',');
                    std::getline(ss, promedio_str, ',');
                    std::getline(ss, maximo_str, ',');
                    std::getline(ss, minimo_str, ',');

                    fecha = trim(fecha);
                    promedio_str = trim(promedio_str);
                    maximo_str = trim(maximo_str);
                    minimo_str = trim(minimo_str);

                    double promedio = promedio_str.empty() ? 0.0 : std::stod(promedio_str);
                    double maximo = maximo_str.empty() ? 0.0 : std::stod(maximo_str);
                    double minimo = minimo_str.empty() ? 0.0 : std::stod(minimo_str);

                    Json::Value result;
                    result["Fecha"] = fecha;
                    result["Promedio"] = promedio;
                    result["Maximo"] = maximo;
                    result["Minimo"] = minimo;
                    result["Riesgoso"] = is_risky(promedio, maximo);

                    Json::StreamWriterBuilder writer;
                    std::string response = Json::writeString(writer, result);

                    // Verificar que WebSocket está abierto antes de escribir
                    if (ws.is_open()) {
                        ws.text(ws.got_text());
                        ws.write(asio::buffer(response));
                    }

                    last_pos = file.tellg();  // Actualizamos la posición de lectura
                    file_updated.store(false);  // Reseteamos la bandera después de procesar los datos

                    data_found = true;  // Marcamos que se encontraron datos nuevos
                    std::cout << "Nuevos datos añadidos \n";
                }
                catch (const std::exception& e) {
                    std::cerr << "Error al procesar línea: " << line << std::endl;
                    std::cerr << "Detalles del error: " << e.what() << std::endl;
                }
            }

            // Si no se encontraron nuevos datos, ponemos el archivo en espera
            if (!data_found) {
                file.close();
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Esperar hasta que haya nuevos datos
                continue;  // Vuelve al inicio del bucle
            }

            file.close();
        }
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "Error: " << se.code().message() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Excepción: " << e.what() << "\n";
    }
}

void monitor_file_update() {
    std::streampos last_pos = 0;
    while (true) {
        std::ifstream file("aire_datos.csv", std::ios::in);
        if (!file.is_open()) {
            std::cerr << "No se pudo abrir el archivo CSV\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        file.seekg(last_pos);  // Nos movemos a la última posición conocida

        std::string line;
        bool new_data = false;

        while (std::getline(file, line)) {
            if (!line.empty()) {
                new_data = true;  // Se encontró un nuevo registro
            }
        }

        if (new_data) {
            file_updated.store(true);  // Marcar que el archivo fue actualizado
        }

        file.close();
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Verificar cada segundo si el archivo fue actualizado
    }
}

int main() {
    try {
        std::thread file_monitor(monitor_file_update);  // Hilo para monitorear el archivo CSV

        asio::io_context ioc;

        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "Servidor WebSocket en puerto 8080.\n";

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread(&websocket_session, std::move(socket)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Excepción en el servidor: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
