# Satranç Motoru Makefile

# Derleyici ve bayraklar
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -mtune=native -I./include
CXXFLAGS += -flto -fno-exceptions -fno-rtti -ffast-math -funroll-loops
CXXFLAGS += -fomit-frame-pointer -fstrict-aliasing -finline-functions
CXXFLAGS += -fprefetch-loop-arrays -fno-stack-protector
CXXFLAGS += -DNDEBUG -DUSE_POPCNT -DUSE_PREFETCH
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -pthread -flto

# Hedef çalıştırılabilir
TARGET = SatrancMotoru

# Dizinler
SRCDIR = src
OBJDIR = obj
INCDIR = include

# Kaynak ve nesne dosyaları
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

# Ana hedef
all: $(TARGET)

# Çalıştırılabilir oluştur
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Nesne dosyaları oluştur
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Nesne dizini oluştur
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Temizlik
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Yeniden derleme
rebuild: clean all

# Debug modu
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -I./include -DDEBUG
debug: clean all

# Performans profili
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean all

# Çalıştır
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild debug profile run