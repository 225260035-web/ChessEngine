#include <iostream>
#include "Tahta.h"
#include "AramaMotoru.h"
#include "HamleUretici.h"
#include "Degerlendirici.h"

int main() {
    std::cout << "Chess Engine Castling Demo\n";
    std::cout << "==========================\n\n";
    
    // Create a position where castling is the best move
    Tahta tahta;
    tahta.fenKur("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    
    std::cout << "Current position:\n";
    tahta.yazdir();
    
    std::cout << "White can castle short: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White can castle long: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n\n";
    
    // Let the engine think
    AramaMotoru motor;
    
    for (int depth = 1; depth <= 6; depth++) {
        std::cout << "\nSearching at depth " << depth << "...\n";
        Hamle bestMove = motor.enIyiHamleyiBul(tahta, depth, std::chrono::milliseconds(5000));
        
        std::cout << "Best move: " << bestMove.notasyon();
        if (bestMove.tur == HamleTuru::KISA_ROK) {
            std::cout << " (Short castle!)";
        } else if (bestMove.tur == HamleTuru::UZUN_ROK) {
            std::cout << " (Long castle!)";
        }
        std::cout << "\n";
    }
    
    // Now make the best move at depth 5
    std::cout << "\nFinal decision at depth 5:\n";
    Hamle finalMove = motor.enIyiHamleyiBul(tahta, 5, std::chrono::milliseconds(5000));
    
    std::cout << "Playing: " << finalMove.notasyon() << "\n\n";
    tahta.hamleYap(finalMove);
    
    std::cout << "Position after move:\n";
    tahta.yazdir();
    
    return 0;
}