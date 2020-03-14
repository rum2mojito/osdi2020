ARMGNU = aarch64-linux-gnu
COPS = -Wall -nostdlib -nostdinc
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: kernel8.img

start.o: start.S
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

%.o: %.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	$(ARMGNU)-ld -T linker.ld -o kernel8.elf start.o $(OBJS) 
	$(ARMGNU)-objcopy -O binary kernel8.elf $@

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio
