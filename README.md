# Satranç Motoru - C++ / SFML

Modern C++ ve SFML kütüphanesi kullanılarak geliştirilmiş, görsel arayüze sahip satranç motoru.

## 🎮 Özellikler

- **Bitboard tabanlı** hızlı hamle üretimi
- **Alpha-Beta budama** ile optimize edilmiş arama algoritması
- **Transposition Table** (Geçişli pozisyon tablosu) desteği
- **Zobrist Hashing** ile pozisyon tanımlama
- **SFML tabanlı** modern görsel arayüz
- **Sürükle-bırak** hamle desteği
- Tüm satranç kuralları (rok, en passant, terfi)
- **Türkçe** kod yapısı ve yorumlar

## 🛠️ Gereksinimler

- C++17 veya üzeri derleyici (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 veya üzeri
- SFML 2.5 veya üzeri
- Thread desteği olan işletim sistemi

### Ubuntu/Debian için kurulum:
```bash
sudo apt update
sudo apt install build-essential cmake libsfml-dev
```

### Fedora için kurulum:
```bash
sudo dnf install gcc-c++ cmake SFML-devel
```

### macOS için kurulum (Homebrew):
```bash
brew install cmake sfml
```

### Windows için kurulum:
1. [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) yükleyin
2. [CMake](https://cmake.org/download/) yükleyin
3. [SFML](https://www.sfml-dev.org/download.php) indirip uygun dizine çıkarın

## 🚀 Derleme ve Çalıştırma

### Linux/macOS:
```bash
# Proje dizinine gidin
cd satranc_cpp

# Build dizini oluşturun
mkdir build && cd build

# CMake ile yapılandırın
cmake ..

# Derleyin
make -j$(nproc)

# Çalıştırın
./SatrancMotoru
```

### Windows (Visual Studio):
```bash
# Proje dizinine gidin
cd satranc_cpp

# Build dizini oluşturun
mkdir build && cd build

# CMake ile Visual Studio projesi oluşturun
cmake .. -G "Visual Studio 16 2019" -A x64

# Visual Studio'da açın veya komut satırından derleyin
cmake --build . --config Release

# Çalıştırın
Release\SatrancMotoru.exe
```

## 📁 Proje Yapısı

```
satranc_cpp/
├── CMakeLists.txt          # CMake yapılandırma dosyası
├── README.md               # Bu dosya
├── include/                # Başlık dosyaları
│   ├── Sabitler.h         # Sabit değerler ve yardımcı fonksiyonlar
│   ├── BitboardYardimci.h # Bitboard işlemleri
│   ├── Hamle.h            # Hamle veri yapısı
│   ├── Tahta.h            # Tahta sınıfı
│   ├── HamleUretici.h     # Hamle üretimi
│   ├── Degerlendirici.h   # Pozisyon değerlendirme
│   ├── AramaMotoru.h      # Arama algoritması
│   ├── Arayuz.h           # SFML arayüz
│   └── Oyun.h             # Oyun yönetimi
├── src/                    # Kaynak dosyaları
│   ├── ana.cpp            # Ana program
│   ├── BitboardYardimci.cpp
│   ├── Tahta.cpp
│   ├── HamleUretici.cpp
│   ├── Degerlendirici.cpp
│   ├── AramaMotoru.cpp
│   ├── Arayuz.cpp
│   └── Oyun.cpp
└── kaynaklar/             # Görsel ve diğer kaynaklar
    └── resimler/          # Taş görselleri
```

## 🎯 Kullanım

### Oyun Kontrolleri:
- **Sol tık**: Taş seçimi ve hamle yapma
- **Sürükle-bırak**: Alternatif hamle yöntemi
- **ESC**: Oyundan çıkış
- **R**: Yeni oyun başlatma
- **U**: Son hamleyi geri alma

### Arayüz Özellikleri:
- Seçili taş vurgulanır
- Legal hamleler yeşil noktalarla gösterilir
- Son hamle sarı ile vurgulanır
- Şah durumu kırmızı ile gösterilir
- Motor düşüncesi ve istatistikler panel üzerinde görüntülenir

## 🔧 Motor Ayarları

`ana.cpp` içinde ayarlanabilir parametreler:

```cpp
OyunAyarlari ayarlar;
ayarlar.motorDerinlik = 6;      // Arama derinliği (4-10 arası önerilir)
ayarlar.ttBoyutMB = 64;         // Transposition table boyutu (MB)
ayarlar.beyazSure = 10;         // Beyaz için süre (dakika)
ayarlar.siyahSure = 10;         // Siyah için süre (dakika)
```

## 📊 Performans

- **Hamle üretimi**: ~5-10 milyon hamle/saniye
- **Arama hızı**: ~1-3 milyon düğüm/saniye
- **Ortalama derinlik**: 6-8 katman (normal sürede)
- **Bellek kullanımı**: ~100-200 MB (TT boyutuna bağlı)

## 🤝 Katkıda Bulunma

1. Projeyi fork edin
2. Feature branch oluşturun (`git checkout -b ozellik/YeniOzellik`)
3. Değişikliklerinizi commit edin (`git commit -m 'Yeni özellik eklendi'`)
4. Branch'e push edin (`git push origin ozellik/YeniOzellik`)
5. Pull Request oluşturun

## 📝 Lisans

Bu proje MIT lisansı altında lisanslanmıştır. Detaylar için `LICENSE` dosyasına bakın.

## 🙏 Teşekkürler

- SFML kütüphanesi geliştiricilerine
- Satranç programlama topluluğuna
- Chessprogramming wiki'ye katkıda bulunanlara