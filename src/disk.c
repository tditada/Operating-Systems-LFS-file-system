#include "disk.h"

#define BIT(i) (1 << (i))

static void sendComm(int ata, int rdwr, unsigned short sector);
static void __read(int ata, char * ans, unsigned short sector, int offset, int count);
static void __write(int ata, char * msg, int bytes, unsigned short sector, int offset);

static unsigned short getDataRegister(int ata);
static void writeDataToRegister(int ata, char upper, char lower);

static void translateBytes(char ans[], unsigned short sector);

void ata_normalize(unsigned short* sector, int* offset) {
	if (*offset >= SECTOR_SIZE) {
		*sector += (*offset / SECTOR_SIZE);
		*offset %= SECTOR_SIZE;
	}
}

void ata_read(int ata, char* ans, int bytes, unsigned short sector, int offset) {
	mt_cli();
	if (ata != ATA0 && ata != ATA1) {
		printk("Disk %d does not exist - [%d, %d]\n", ata, sector, offset);
		return;
	}
	while (bytes != 0) {
		if (offset >= SECTOR_SIZE) {
			sector += (offset / SECTOR_SIZE);
			offset %= SECTOR_SIZE;
		}
		int size;
		if (offset + bytes > SECTOR_SIZE) { // read remaining part from the sector
			size = SECTOR_SIZE - offset;
			__read(ata, ans, sector, offset, size);
			sector++;
			offset = 0;
			bytes -= size;
			ans += size;
		} else { // The remaining msg fits in the actual sector
			size = bytes;
			__read(ata, ans, sector, offset, size);
			offset += size;
			bytes = 0;
			ans += size;
		}
	}
	mt_sti();
}

void __read(int ata, char * ans, unsigned short sector, int offset, int count) {
	char tmp[SECTOR_SIZE];
	sendComm(ata, LBA_READ, sector);
	/* Read sector*/
	int b;
	unsigned short data;
	for (b = 0; b < SECTOR_SIZE; b += 2) {
		data = getDataRegister(ata);
		translateBytes(tmp + b, data);
	}
	int i;
	for (i = 0; i < count; i++) {
		ans[i] = tmp[offset + i];
	}
}

void translateBytes(char * ans, unsigned short databyte) {
	ans[0] = databyte & 0xff;
	ans[1] = (databyte >> 8) & 0xFF;
}

void ata_write(int ata, char * msg, int bytes, unsigned short sector,
		int offset) {
	mt_cli();
	if (ata != ATA0 && ata != ATA1) {
		printk("Disk %d does not exist - [%d, %d]\n", ata, sector, offset);
		return;
	}
	char* ans = (char*) msg;
	while (bytes != 0) {
		if (offset >= SECTOR_SIZE) {
			sector += (offset / SECTOR_SIZE);
			offset %= SECTOR_SIZE;
		}
		int size;
		int i;
		if (offset + bytes > SECTOR_SIZE) { // Fill sector
			size = SECTOR_SIZE - offset;
			__write(ata, ans, size, sector, offset);
			for (i = 0; i < 900000; i++) {
			}
			sector++;
			offset = 0;
			bytes -= size;
			ans += size;
		} else { // The remaining msg fits in the actual sector
			size = bytes;
			__write(ata, ans, size, sector, offset);
			for (i = 0; i < 900000; i++) {
			}
			offset += size;
			bytes = 0;
			ans += size;
		}
	}
	mt_sti();
}

void __write(int ata, char * msg, int bytes, unsigned short sector, int offset) {
	char tmp[SECTOR_SIZE];
	int i;
	// Read actual sector because ATA always writes a complete sector!
	// Don't overwrite previous values!
	__read(ata, tmp, sector, 0, SECTOR_SIZE);
	for (i = 0; i < bytes; i++) {
		tmp[offset + i] = msg[i];
	}
	sendComm(ata, LBA_WRITE, sector);
	// Write updated sector
	for (i = 0; i < SECTOR_SIZE; i += 2) {
		writeDataToRegister(ata, tmp[i + 1], tmp[i]);
	}
}

void writeDataToRegister(int ata, char upper, char lower) {
	mt_cli();
	unsigned short out;
	// Wait for driver's ready signal.
	while (!(inw(ata + WIN_REG7) & BIT(3))) {
		//printk("Writing data register - Waiting for disk to be ready...\n");;
	}
	out = upper << 8 | (lower & 0xff);
	outw(ata + WIN_REG0, out);
	mt_sti();
}

unsigned short getDataRegister(int ata) {
	mt_cli();
	unsigned short ans;
	// Wait for driver's ready signal.
	while (!(inw(ata + WIN_REG7) & BIT(3))) {
		//printk("Reading data register - Waiting for disk to be ready...\n");
	}
	ans = inw(ata + WIN_REG0);
	mt_sti();
	return ans;
}

unsigned short getErrorRegister(int ata) {
	unsigned short rta = inb(ata + WIN_REG1) & 0x00000FFFF;
	return rta;
}

void sendComm(int ata, int rdwr, unsigned short sector) {
	outb(ata + WIN_REG1, 0);
	outb(ata + WIN_REG2, 1); // Set count register sector in 1

	outb(ata + WIN_REG3, (unsigned char) sector); // LBA low
	outb(ata + WIN_REG4, (unsigned char) (sector >> 8)); // LBA mid
	outb(ata + WIN_REG5, (unsigned char) (sector >> 16)); // LBA high
	outb(ata + WIN_REG6, 0xE0 | (ata << 4) | ((sector >> 24) & 0x0F)); // Set LBA bit in 1 and the rest in 0
	// Set command
	outb(ata + WIN_REG7, rdwr);
}

unsigned short ata_getStatusRegister(int ata) {
	unsigned short rta;
	rta = inb(ata + WIN_REG7) & 0x00000FFFF;
	return rta;
}

void identifyDevice(int ata) {
	outb(ata + WIN_REG6, 0);
	outb(ata + WIN_REG7, WIN_IDENTIFY);
}

disk_data disk_identify() {
	identifyDevice(ATA0);

	disk_data data;

	unsigned short reg = 0;
	int i;
	/*Word 60 y 61 contienen el LBA total*/
	short word60, word61;

	printk("Identifying ATA0 device.\n\n");
	for (i = 0; i < 255; i++) {
		reg = getDataRegister(ATA0);
		if (i == 0) {
			data.removable = reg & BIT(7);
			data.ata = !(reg & BIT(15));
		} else if (i >= 27 && i <= 46) {
			if (reg > 0) {
				sprintf(data.id, "%c%c", (reg & 0xFF00) >> 8, reg & 0xFF);
			}
		} else if (i == 49) {
			data.lba = reg & BIT(9);
		} else if (i == 60) {
			word60 = reg;
		} else if (i == 61) {
			word61 = reg;
		}
	}
	data.size = ((word61 << 14) + word60) / SECTOR_SIZE;

	printk("\tRemovable: %s\n", data.removable? "Yes" : "No");
	printk("\tATA: %s\n", data.ata? "No" : "Yes");
	printk("\tID: %s\n", data.id);
	printk("\tLBA: %s\n", data.lba? "Yes" : "No");
	printk("\tSize: %dMB\n", data.size);

	return data;
}
