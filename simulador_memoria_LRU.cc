#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <cstdlib>

/**
 * ###
 * ###     S I M U L A D O R    D E    M E M Ó R I A
 * ### "Traduzido" em C++ a partir do código em Python do 
 * ### Prof. Filipo - github.com/ProfessorFilipo/MemSim/
 * ###
 * ### Algoritmo implementado: LRU (Least Recently Used)
 */

const std::string NOME_ALGORITMO = "LRU";

class Frame {
public:
    int idFrame;
    std::optional<int> paginaAlocada; // Armazena o número da página ou std::nullopt se estiver vazio
    int ultimoAcesso; //? Marca temporal do último acesso, usada para escolher a vítima do LRU

    Frame(int idFrame) : idFrame(idFrame), paginaAlocada(std::nullopt), ultimoAcesso(0) {
        // Dica para os alunos: vocês podem adicionar atributos aqui para ajudar no algoritmo (ex: timestamp, contador)
    }
};

// Guarda o estado de um passo da simulação para exportação em JSON
struct PassoHistorico {
    int passo;
    int pagina;
    bool hit;
    std::optional<int> frameAlterado;
    bool novo; // true se a página foi colocada em um frame vazio (não houve substituição)
    int totalAcessosAcumulado;
    int totalFaultsAcumulado;
    // Snapshot dos frames após o acesso: pares (paginaAlocada ou -1, ultimoAcesso)
    std::vector<std::pair<int,int>> framesSnapshot;
};

class TabelaPaginas {
public:
    std::vector<Frame> frames;
    int totalPageFaults;
    int totalAcessos;

    TabelaPaginas(int numFrames) : totalPageFaults(0), totalAcessos(0) {
        // Inicializa a memória física com a quantidade de frames especificada
        for (int i = 0; i < numFrames; ++i) {
            frames.emplace_back(i);
        }
    }

    std::pair<bool, int> acessarPagina(int numeroPagina) {
        totalAcessos++;

        // 1. Verificar se a página já está em algum frame (Hit)
        for (auto& frame : frames) {
            if (frame.paginaAlocada.has_value() && frame.paginaAlocada.value() == numeroPagina) {
                frame.ultimoAcesso = totalAcessos; //? Atualiza o "tempo" do último uso
                return {true, frame.idFrame};
            }
        }

        // 2. Se não encontrou, ocorreu um Page Fault!
        totalPageFaults++;

        // 3. Verificar se existe algum frame vazio disponível
        for (auto& frame : frames) {
            if (!frame.paginaAlocada.has_value()) {
                frame.paginaAlocada = numeroPagina;
                frame.ultimoAcesso = totalAcessos;
                return {false, frame.idFrame};
            }
        }

        // 4. Memória cheia: Aplicar algoritmo de substituição LRU
        int frameVitimaId = substituirPagina(numeroPagina);
        return {false, frameVitimaId};
    }

    int substituirPagina(int novaPagina) {
        // Algoritmo LRU: escolhe o frame com o menor 'ultimoAcesso' (o que está há mais tempo sem uso)
        int frameVitimaIdx = 0;
        for (int i = 1; i < (int)frames.size(); ++i) {
            if (frames[i].ultimoAcesso < frames[frameVitimaIdx].ultimoAcesso) {
                frameVitimaIdx = i;
            }
        }
        frames[frameVitimaIdx].paginaAlocada = novaPagina;
        frames[frameVitimaIdx].ultimoAcesso = totalAcessos;
        return frames[frameVitimaIdx].idFrame;
    }

    void imprimirMapaMemoria(int passo, int paginaAcessada, bool foiHit, std::optional<int> frameAlterado = std::nullopt) {
        std::string status = foiHit ? "Hit" : "Page Fault";
        std::cout << "\n--- Passo " << passo << ": Acesso à Página " << paginaAcessada << " (" << status << ") ---" << std::endl;

        for (const auto& frame : frames) {
            std::string conteudo = frame.paginaAlocada.has_value() ? "Página " + std::to_string(frame.paginaAlocada.value()) : "[Vazio]";
            std::string marcador = "";
            if (frameAlterado.has_value() && frame.idFrame == frameAlterado.value() && !foiHit) {
                marcador = " <-- Alterado";
            }
            std::cout << "[Frame " << frame.idFrame << "]: " << conteudo << marcador << std::endl;
        }

        std::cout << std::string(40, '-') << std::endl;
    }

    // Captura um snapshot do estado atual dos frames: (paginaAlocada ou -1, ultimoAcesso)
    std::vector<std::pair<int,int>> snapshot() const {
        std::vector<std::pair<int,int>> s;
        for (const auto& frame : frames) {
            int pag = frame.paginaAlocada.has_value() ? frame.paginaAlocada.value() : -1;
            s.push_back({pag, frame.ultimoAcesso});
        }
        return s;
    }
};

class Simulador {
private:
    std::string caminhoArquivo;

public:
    Simulador(std::string caminhoArquivo) : caminhoArquivo(caminhoArquivo) {}

    void executar() {
        std::ifstream arquivo(caminhoArquivo);
        if (!arquivo.is_open()) {
            std::cout << "Erro: O arquivo '" << caminhoArquivo << "' não foi encontrado." << std::endl;
            return;
        }

        std::vector<std::string> linhas;
        std::string linha;
        while (std::getline(arquivo, linha)) {
            size_t first = linha.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            size_t last = linha.find_last_not_of(" \t\r\n");
            std::string trimmed = linha.substr(first, (last - first + 1));

            if (!trimmed.empty() && trimmed[0] != '#') {
                linhas.push_back(trimmed);
            }
        }
        arquivo.close();

        if (linhas.empty()) {
            std::cout << "Erro: Arquivo de entrada vazio." << std::endl;
            return;
        }

        int numFrames = std::stoi(linhas[0]);
        TabelaPaginas tabelaPaginas(numFrames);

        std::cout << "Iniciando simulação com " << numFrames << " frames disponíveis." << std::endl;
        std::cout << "Algoritmo: " << NOME_ALGORITMO << std::endl;
        std::cout << std::string(40, '=') << std::endl;

        std::vector<PassoHistorico> historico;

        int passo = 1;
        for (size_t i = 1; i < linhas.size(); ++i) {
            int numeroPagina = std::stoi(linhas[i]);

            // Verifica antecipadamente se a página vai para um frame vazio,
            // para diferenciar "novo" (frame vazio) de "substituição" no histórico
            bool indoParaFrameVazio = false;
            for (const auto& frame : tabelaPaginas.frames) {
                if (frame.paginaAlocada.has_value() && frame.paginaAlocada.value() == numeroPagina) {
                    indoParaFrameVazio = false;
                    break;
                }
                if (!frame.paginaAlocada.has_value()) {
                    indoParaFrameVazio = true;
                }
            }

            std::pair<bool, int> resultado = tabelaPaginas.acessarPagina(numeroPagina);
            bool foiHit = resultado.first;
            int frameId = resultado.second;

            tabelaPaginas.imprimirMapaMemoria(passo, numeroPagina, foiHit, frameId);

            PassoHistorico h;
            h.passo = passo;
            h.pagina = numeroPagina;
            h.hit = foiHit;
            h.frameAlterado = foiHit ? std::nullopt : std::optional<int>(frameId);
            h.novo = !foiHit && indoParaFrameVazio;
            h.totalAcessosAcumulado = tabelaPaginas.totalAcessos;
            h.totalFaultsAcumulado = tabelaPaginas.totalPageFaults;
            h.framesSnapshot = tabelaPaginas.snapshot();
            historico.push_back(h);

            passo++;
        }

        std::cout << "\n================ STATS FINAIS ================" << std::endl;
        std::cout << "Total de Acessos: " << tabelaPaginas.totalAcessos << std::endl;
        std::cout << "Total de Page Faults: " << tabelaPaginas.totalPageFaults << std::endl;
        double taxaFaults = 0.0;
        if (tabelaPaginas.totalAcessos > 0) {
            taxaFaults = (static_cast<double>(tabelaPaginas.totalPageFaults) / tabelaPaginas.totalAcessos) * 100.0;
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Taxa de Page Faults: " << taxaFaults << "%" << std::endl;
        }
        std::cout << "==============================================" << std::endl;

        exportarJsonEAbrirHtml(numFrames, historico, taxaFaults);
    }

    // Gera um arquivo JS (dados.js) com o histórico e abre visualizador.html no navegador padrão
    void exportarJsonEAbrirHtml(int numFrames, const std::vector<PassoHistorico>& historico, double taxaFaults) {
        std::ofstream out("dados_lru.js");
        out << std::fixed << std::setprecision(2);
        out << "// Arquivo gerado automaticamente pelo simulador C++\n";
        out << "const DADOS_SIMULACAO = {\n";
        out << "  algoritmo: \"" << NOME_ALGORITMO << "\",\n";
        out << "  numFrames: " << numFrames << ",\n";
        out << "  taxaFaults: " << taxaFaults << ",\n";
        out << "  historico: [\n";

        for (size_t i = 0; i < historico.size(); ++i) {
            const auto& h = historico[i];
            out << "    {\n";
            out << "      passo: " << h.passo << ",\n";
            out << "      pagina: " << h.pagina << ",\n";
            out << "      hit: " << (h.hit ? "true" : "false") << ",\n";
            out << "      frameAlterado: " << (h.frameAlterado.has_value() ? std::to_string(h.frameAlterado.value()) : "null") << ",\n";
            out << "      novo: " << (h.novo ? "true" : "false") << ",\n";
            out << "      acessos: " << h.totalAcessosAcumulado << ",\n";
            out << "      faults: " << h.totalFaultsAcumulado << ",\n";
            out << "      frames: [";
            for (size_t f = 0; f < h.framesSnapshot.size(); ++f) {
                out << "{pagina: " << h.framesSnapshot[f].first
                    << ", ultimoAcesso: " << h.framesSnapshot[f].second << "}";
                if (f + 1 < h.framesSnapshot.size()) out << ", ";
            }
            out << "]\n";
            out << "    }" << (i + 1 < historico.size() ? "," : "") << "\n";
        }

        out << "  ]\n";
        out << "};\n";
        out.close();

        std::cout << "\nArquivo 'dados_lru.js' gerado com sucesso." << std::endl;
        std::cout << "Abrindo visualizador HTML..." << std::endl;

#ifdef _WIN32
        std::system("start visualizador.html");
#elif __APPLE__
        std::system("open visualizador.html");
#else
        std::system("xdg-open visualizador.html");
#endif
    }
};

int main(int argc, char* argv[]) {
    std::string arquivoEntrada = (argc > 1) ? argv[1] : "entrada.txt";
    Simulador simulador(arquivoEntrada);
    simulador.executar();
    return 0;
}