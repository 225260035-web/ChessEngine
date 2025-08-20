#include <iostream>
#include "Tahta.h"
#include "AramaMotoru.h"
#include "HamleUretici.h"
#include "Degerlendirici.h"

void testCastlingRights() {
    std::cout << "Testing castling rights preservation during search...\n\n";
    
    // Test 1: Position where castling is possible (no pieces between king and rooks)
    Tahta tahta;
    tahta.fenKur("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    std::cout << "Test 1: Position with clear castling paths\n";
    tahta.yazdir(); // Print the board to see the position
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "Black short castle: " << (tahta.kisaRokYapabilirMi(Renk::SIYAH) ? "YES" : "NO") << "\n";
    std::cout << "Black long castle: " << (tahta.uzunRokYapabilirMi(Renk::SIYAH) ? "YES" : "NO") << "\n";
    
    // Test if castling moves are generated
    std::cout << "\nGenerated moves for white:\n";
    HamleListesi hamleler;
    HamleUretici::tumHamleleriUret(tahta, hamleler);
    
    bool foundShortCastle = false;
    bool foundLongCastle = false;
    
    for (int i = 0; i < hamleler.getBoyut(); i++) {
        const Hamle& hamle = hamleler[i];
        if (hamle.tur == HamleTuru::KISA_ROK) {
            std::cout << "Found SHORT CASTLE: " << hamle.notasyon() << "\n";
            foundShortCastle = true;
        } else if (hamle.tur == HamleTuru::UZUN_ROK) {
            std::cout << "Found LONG CASTLE: " << hamle.notasyon() << "\n";
            foundLongCastle = true;
        }
    }
    
    if (!foundShortCastle && tahta.kisaRokYapabilirMi(Renk::BEYAZ)) {
        std::cout << "ERROR: Short castle is possible but NOT generated!\n";
    }
    if (!foundLongCastle && tahta.uzunRokYapabilirMi(Renk::BEYAZ)) {
        std::cout << "ERROR: Long castle is possible but NOT generated!\n";
    }
    
    std::cout << "Total moves generated: " << hamleler.getBoyut() << "\n\n";
    
    // Test if castling moves are legal
    std::cout << "Testing castling legality:\n";
    for (int i = 0; i < hamleler.getBoyut(); i++) {
        const Hamle& hamle = hamleler[i];
        if (hamle.tur == HamleTuru::KISA_ROK || hamle.tur == HamleTuru::UZUN_ROK) {
            bool legal = tahta.legalMi(hamle);
            std::cout << "Move " << hamle.notasyon() << " is " << (legal ? "LEGAL" : "ILLEGAL") << "\n";
            
            // Try making the move and see if it leaves king in check
            Tahta tmpTahta = tahta;
            tmpTahta.hamleYap(hamle);
            bool inCheck = tmpTahta.sahCekildiMi(tersRenk(tmpTahta.getSira()));
            std::cout << "  After move, own king in check: " << (inCheck ? "YES" : "NO") << "\n";
            
            if (!legal && !inCheck) {
                std::cout << "  ERROR: Move should be legal but marked as illegal!\n";
            }
        }
    }
    std::cout << "\n";
    
    // Test 2: Make a move and undo it - castling rights should remain
    HamleListesi hamleler2;
    HamleUretici::tumHamleleriUret(tahta, hamleler2);
    
    if (hamleler2.getBoyut() > 0) {
        Hamle hamle = hamleler2[0];
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
    
    // Test 3: Engine search shouldn't affect castling rights
    std::cout << "Test 3: Engine search\n";
    std::cout << "Before engine search:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    
    // Run engine search
    AramaMotoru motor;
    Hamle bestMove = motor.enIyiHamleyiBul(tahta, 3, std::chrono::milliseconds(1000));
    
    std::cout << "\nAfter engine search:\n";
    std::cout << "White short castle: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "Engine suggested move: " << bestMove.notasyon();
    if (bestMove.tur == HamleTuru::KISA_ROK) {
        std::cout << " (Short castle!)";
    } else if (bestMove.tur == HamleTuru::UZUN_ROK) {
        std::cout << " (Long castle!)";
    }
    std::cout << "\n\n";
    
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
    std::cout << "White long castle: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n\n";
    
    // Test 5: Position where castling is clearly the best move
    std::cout << "Test 5: Position where castling should be best\n";
    // Position with open f1, g1 squares and king under slight pressure
    tahta.fenKur("r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 6");
    
    std::cout << "Before search:\n";
    tahta.yazdir();
    std::cout << "White can castle short: " << (tahta.kisaRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    std::cout << "White can castle long: " << (tahta.uzunRokYapabilirMi(Renk::BEYAZ) ? "YES" : "NO") << "\n";
    
    bestMove = motor.enIyiHamleyiBul(tahta, 5, std::chrono::milliseconds(3000));
    
    std::cout << "\nEngine's best move: " << bestMove.notasyon();
    if (bestMove.tur == HamleTuru::KISA_ROK) {
        std::cout << " (Short castle - CORRECT!)";
    } else if (bestMove.tur == HamleTuru::UZUN_ROK) {
        std::cout << " (Long castle!)";
    } else {
        std::cout << " (Not castling - might be missing the opportunity)";
    }
    std::cout << "\n";
    
    // Make the move to see the result
    tahta.hamleYap(bestMove);
    std::cout << "\nAfter engine's move:\n";
    tahta.yazdir();
    
    // Test 6: Debug why castling is not chosen
    std::cout << "\nTest 6: Detailed castling analysis\n";
    tahta.fenKur("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    // Manually evaluate castling vs other moves
    std::cout << "Evaluating key moves:\n";
    
    // Evaluate e1g1 (short castle)
    Hamle shortCastle(E1, G1, TasTuru::SAH, TasTuru::YOK, HamleTuru::KISA_ROK);
    tahta.hamleYap(shortCastle);
    // Always evaluate from white's perspective, then negate if it's black's turn
    int rawEval = Degerlendirici::degerlendir(tahta);
    int castleEval = tahta.getSira() == Renk::BEYAZ ? rawEval : -rawEval;
    tahta.hamleGeriAl(shortCastle);
    std::cout << "After e1g1 (short castle): eval = " << castleEval << " (raw = " << rawEval << ")\n";
    
    // Evaluate d2d4
    Hamle d2d4(D2, D4, TasTuru::PIYON, TasTuru::YOK, HamleTuru::IKI_KARE);
    tahta.hamleYap(d2d4);
    rawEval = Degerlendirici::degerlendir(tahta);
    int d2d4Eval = tahta.getSira() == Renk::BEYAZ ? rawEval : -rawEval;
    tahta.hamleGeriAl(d2d4);
    std::cout << "After d2d4: eval = " << d2d4Eval << " (raw = " << rawEval << ")\n";
    
    // Evaluate e2e4
    Hamle e2e4(E2, E4, TasTuru::PIYON, TasTuru::YOK, HamleTuru::IKI_KARE);
    tahta.hamleYap(e2e4);
    rawEval = Degerlendirici::degerlendir(tahta);
    int e2e4Eval = tahta.getSira() == Renk::BEYAZ ? rawEval : -rawEval;
    tahta.hamleGeriAl(e2e4);
    std::cout << "After e2e4: eval = " << e2e4Eval << " (raw = " << rawEval << ")\n";
    
    std::cout << "\nCastling advantage: " << (castleEval - std::max(d2d4Eval, e2e4Eval)) << " centipawns\n";
    
    // Test 7: Check what happens at depth 1
    std::cout << "\nTest 7: Engine evaluation at depth 1\n";
    AramaMotoru motor2;
    Hamle bestMoveDepth1 = motor2.enIyiHamleyiBul(tahta, 1, std::chrono::milliseconds(5000));
    std::cout << "Best move at depth 1: " << bestMoveDepth1.notasyon();
    if (bestMoveDepth1.tur == HamleTuru::KISA_ROK) {
        std::cout << " (Short castle!)";
    } else if (bestMoveDepth1.tur == HamleTuru::UZUN_ROK) {
        std::cout << " (Long castle!)";
    }
    std::cout << "\n";
    
    // Test 8: Analyze why castling loses favor at higher depths
    std::cout << "\nTest 8: Detailed move analysis\n";
    tahta.fenKur("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    // Test castling followed by opponent's response
    std::cout << "After 1.O-O (e1g1):\n";
    Hamle whiteShortCastle(E1, G1, TasTuru::SAH, TasTuru::YOK, HamleTuru::KISA_ROK);
    tahta.hamleYap(whiteShortCastle);
    
    // Generate black's moves
    HamleListesi blackMoves;
    HamleUretici::tumHamleleriUret(tahta, blackMoves);
    
    // Find black's castling
    for (int i = 0; i < blackMoves.getBoyut(); i++) {
        if (blackMoves[i].tur == HamleTuru::KISA_ROK) {
            std::cout << "  After 1...O-O (e8g8):\n";
            tahta.hamleYap(blackMoves[i]);
            int eval = Degerlendirici::degerlendir(tahta);
            std::cout << "    Evaluation: " << eval << " (from white's perspective)\n";
            tahta.hamleGeriAl(blackMoves[i]);
            break;
        }
    }
    
    tahta.hamleGeriAl(whiteShortCastle);
    
    // Test d2d4 followed by opponent's response
    std::cout << "\nAfter 1.d4 (d2d4):\n";
    Hamle d4(D2, D4, TasTuru::PIYON, TasTuru::YOK, HamleTuru::IKI_KARE);
    tahta.hamleYap(d4);
    
    // Check a typical response
    Hamle d5(D7, D5, TasTuru::PIYON, TasTuru::YOK, HamleTuru::IKI_KARE);
    std::cout << "  After 1...d5 (d7d5):\n";
    tahta.hamleYap(d5);
    int eval = Degerlendirici::degerlendir(tahta);
    std::cout << "    Evaluation: " << eval << " (from white's perspective)\n";
    
    tahta.hamleGeriAl(d5);
    tahta.hamleGeriAl(d4);
}

int main() {
    testCastlingRights();
    return 0;
}