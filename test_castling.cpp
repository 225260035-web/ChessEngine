#include <iostream>
#include "Tahta.h"
#include "AramaMotoru.h"
#include "Oyun.h"

void testCastlingRights() {
    std::cout << "Testing castling rights preservation during search...\n\n";
    
    // Test 1: Starting position - all castling rights should be available
    Tahta tahta;
    tahta.baslangicPozisyonu();
    
    std::cout << "Test 1: Starting position\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "Black short castle: " << (tahta.kisaRokYapabilirMi(Renk::SIYAH) ? "YES" : "NO") << "\n";
    std::cout << "Black long castle: " << (tahta.uzunRokYapabilirMi(Renk::SIYAH) ? "YES" : "NO") << "\n\n";
    
    // Test 2: Make a move and undo it - castling rights should remain
    HamleListesi hamleler;
    HamleUretici::tumHamleleriUret(tahta, hamleler);
    
    if (hamleler.getBoyut() > 0) {
        Hamle hamle = hamleler[0];
        std::cout << "Test 2: Making and undoing move " << hamle.notasyon() << "\n";
        
        // Save initial state
        bool whiteShortBefore = tahta.kisaRokYapabilirMi(Renk::BEYAZ);
        bool whiteLongBefore = tahta.uzunRokYapabilirMi(Renk::BEYAZ);
        bool blackShortBefore = tahta.kisaRokYapabilirMi(Renk::SIYAH);
        bool blackLongBefore = tahta.uzunRokYapabilirMi(Renk::SIYAH);
        
        // Make and undo move
        tahta.hamleYap(hamle);
        tahta.hamleGeriAl(hamle);
        
        // Check if rights are preserved
        std::cout << "White short castle preserved: " << 
            ((whiteShortBefore == tahta.kisaRokYapabilirMi(Renk::BEYAZ)) ? "YES" : "NO") << "\n";
        std::cout << "White long castle preserved: " << 
            ((whiteLongBefore == tahta.uzunRokYapabilirMi(Renk::BEYAZ)) ? "YES" : "NO") << "\n";
        std::cout << "Black short castle preserved: " << 
            ((blackShortBefore == tahta.kisaRokYapabilirMi(Renk::SIYAH)) ? "YES" : "NO") << "\n";
        std::cout << "Black long castle preserved: " << 
            ((blackLongBefore == tahta.uzunRokYapabilirMi(Renk::SIYAH)) ? "YES" : "NO") << "\n\n";
    }
    
    // Test 3: Position where castling is possible
    std::cout << "Test 3: Position with castling possible\n";
    tahta.fenKur("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    std::cout << "Before engine search:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    
    // Run engine search
    AramaMotoru motor;
    Hamle bestMove = motor.enIyiHamleyiBul(tahta, 3, std::chrono::milliseconds(1000));
    
    std::cout << "\nAfter engine search:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "Engine suggested move: " << bestMove.notasyon() << "\n\n";
    
    // Test 4: Test after king move (should lose castling rights)
    std::cout << "Test 4: After king move\n";
    tahta.fenKur("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    // Move white king
    Hamle kingMove(E1, E2, TasTuru::SAH);
    tahta.hamleYap(kingMove);
    
    std::cout << "After king move e1e2:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    
    // Undo the move
    tahta.hamleGeriAl(kingMove);
    
    std::cout << "\nAfter undoing king move:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
}

int main() {
    testCastlingRights();
    return 0;
}