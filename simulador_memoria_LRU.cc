#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <optional>

/**
 * ###
 * ###     S I M U L A D O R    D E    M E M Ó R I A
 * ### "Traduzido" em C++ a partir do código em Python do 
 * ### Prof. Filipo - github.com/ProfessorFilipo/MemSim/
 * ###
 */

class Frame {
public:
    int idFrame;
    std::optional<int> paginaAlocada; // Armazena o número da página ou std::nullopt se estiver vazio
    int ultimoAcesso; //? Variavel global para armazenar último acesso para gerir trocas do LRU

    Frame(int idFrame) : idFrame(idFrame), paginaAlocada(std::nullopt), ultimoAcesso(0) {
        // Dica para os alunos: vocês podem adicionar atributos aqui para ajudar no algoritmo (ex: timestamp, contador)
    }
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
                // TODO: Se necessário para o algoritmo (ex: LRU), atualize metadados aqui.
                frame.ultimoAcesso = totalAcessos; //? Registra o hit 
                return {true, frame.idFrame}; // Retorna (Hit=True, frame_id)
            }
        }

        // 2. Se não encontrou, ocorreu um Page Fault!
        totalPageFaults++;

        // 3. Verificar se existe algum frame vazio disponível
        for (auto& frame : frames) {
            if (!frame.paginaAlocada.has_value()) {
                frame.paginaAlocada = numeroPagina;
                // TODO: Se necessário para o algoritmo, inicialize metadados do frame aqui.
                frame.ultimoAcesso = totalAcessos; //? Registra o ultimo acesso no caso de page fault
                return {false, frame.idFrame}; // Retorna (Hit=False, frame_id)
            }
        }

        // 4. Memória cheia: Aplicar algoritmo de substituição de página
        int frameVitimaId = substituirPagina(numeroPagina);
        return {false, frameVitimaId};
    }

    int substituirPagina(int novaPagina) {
        /**
         * TODO: IMPLEMENTAR PELO GRUPO
         * Esta função deve escolher uma página 'vítima' para ser substituída
         * com base no algoritmo escolhido (FIFO ou LRU), atualizar o frame
         * escolhido com a nova_pagina e retornar o ID do frame que foi alterado.
         */
        int frameEscolhidoId = 0;

        // Escreva a lógica do algoritmo aqui...

        // Exemplo de atualização (substitua pela lógica real):
        // frames[frameEscolhidoId].paginaAlocada = novaPagina;

        return frameEscolhidoId;
    }

    void imprimirMapaMemoria(int passo, int paginaAcessada, bool foiHit, std::optional<int> frameAlterado = std::nullopt) {
        /**
         * TODO: IMPLEMENTAR PELO GRUPO
         * Esta função deve imprimir o estado atual da memória física (frames) no terminal,
         * conforme o padrão visual exigido no enunciado do trabalho.
         */
        std::string status = foiHit ? "Hit" : "Page Fault";
        std::cout << "\n--- Passo " << passo << ": Acesso à Página " << paginaAcessada << " (" << status << ") ---" << std::endl;

        // Exemplo de iteração sobre os frames para os alunos completarem o print:
        for (const auto& frame : frames) {
            std::string conteudo = frame.paginaAlocada.has_value() ? "Página " + std::to_string(frame.paginaAlocada.value()) : "[Vazio]";
            std::string marcador = (frameAlterado.has_value() && frame.idFrame == frameAlterado.value() && !foiHit) ? " <-- Alterado" : "";
            std::cout << "[Frame " << frame.idFrame << "]: " << conteudo << marcador << std::endl;
        }

        std::cout << std::string(40, '-') << std::endl;
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
            // Limpa espaços em branco no início e fim
            size_t first = linha.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            size_t last = linha.find_last_not_of(" \t\r\n");
            std::string trimmed = linha.substr(first, (last - first + 1));

            // Ignora linhas vazias ou comentários
            if (!trimmed.empty() && trimmed[0] != '#') {
                linhas.push_back(trimmed);
            }
        }
        arquivo.close();

        if (linhas.empty()) {
            std::cout << "Erro: Arquivo de entrada vazio." << std::endl;
            return;
        }

        // A primeira linha válida define o número de frames na memória RAM simulada
        int numFrames = std::stoi(linhas[0]);
        TabelaPaginas tabelaPaginas(numFrames);

        std::cout << "Iniciando simulação com " << numFrames << " frames disponíveis." << std::endl;
        std::cout << std::string(40, '=') << std::endl;

        // As linhas seguintes são a sequência de acessos às páginas
        int passo = 1;
        for (size_t i = 1; i < linhas.size(); ++i) {
            int numeroPagina = std::stoi(linhas[i]);

            // Processa o acesso na tabela de páginas
            std::pair<bool, int> resultado = tabelaPaginas.acessarPagina(numeroPagina);
            bool foiHit = resultado.first;
            int frameId = resultado.second;

            // Renderiza o mapa de memória para o aluno ver o passo a passo
            tabelaPaginas.imprimirMapaMemoria(passo, numeroPagina, foiHit, frameId);
            passo++;
        }

        // Exibição das estatísticas finais da simulação
        std::cout << "\n================ STATS FINAIS ================" << std::endl;
        std::cout << "Total de Acessos: " << tabelaPaginas.totalAcessos << std::endl;
        std::cout << "Total de Page Faults: " << tabelaPaginas.totalPageFaults << std::endl;
        if (tabelaPaginas.totalAcessos > 0) {
            double taxaFaults = (static_cast<double>(tabelaPaginas.totalPageFaults) / tabelaPaginas.totalAcessos) * 100.0;
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Taxa de Page Faults: " << taxaFaults << "%" << std::endl;
        }
        std::cout << "==============================================" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Permite passar o arquivo de entrada por argumento de linha de comando ou usa um padrão
    std::string arquivoEntrada = (argc > 1) ? argv[1] : "entrada.txt";
    Simulador simulador(arquivoEntrada);
    simulador.executar();
    return 0;
}
