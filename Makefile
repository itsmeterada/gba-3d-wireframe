# Makefile for GBA development

# Toolchain prefix
PREFIX = /opt/devkitpro/devkitARM/bin/arm-none-eabi-

# Tools
CC = $(PREFIX)gcc
LD = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy

# Directories
SRCDIR = source
INCDIR = include
BLDDIR = build
BINDIR = bin

# Target file
TARGET = $(BINDIR)/my_game.gba

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)

# Object files
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.o,$(SOURCES))

# Flags
CFLAGS = -I$(INCDIR) -mthumb -mthumb-interwork -mlong-calls
LDFLAGS = -specs=gba.specs -mthumb -mthumb-interwork -u _printf_float

# Create bin directory if it doesn't exist
$(shell mkdir -p $(BINDIR))

# Default rule
all: $(TARGET)

# Rule to link the object files
$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $(patsubst %.gba,%.elf,$(TARGET))
	$(OBJCOPY) -O binary $(patsubst %.gba,%.elf,$(TARGET)) $(TARGET)

# Rule to compile the source files
$(BLDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(BLDDIR)/*.o $(BINDIR)/*.elf $(TARGET)

.PHONY: all clean
