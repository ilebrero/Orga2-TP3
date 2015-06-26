/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
  definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "i386.h"

/* Macros */
/* -------------------------------------------------------------------------- */
#define PDE_INDEX(virt, res) \
 	res = virt >> 22;   

#define PTE_INDEX(virt, res) \
	res = (virt << 10) >> 22;

/* Atributos paginas */
/* -------------------------------------------------------------------------- */

#define PG_PRESENT		1
#define PG_READ_WRITE   1
#define PG_USER			0

/* Direcciones fisicas de codigos */
/* -------------------------------------------------------------------------- */
/* En estas direcciones estan los códigos de todas las tareas. De aqui se
 * copiaran al destino indicado por TASK_<i>_CODE_ADDR.
 */

void mmu_inicializar(){
	paginas_libres.cantidad 	 = 768; //1024-256
	paginas_libres.primera_libre = (uint*) 0x100000;
}

void* mmu_gimme_gimme_page_wachin(){
	void* result = NULL;
	if (paginas_libres.cantidad > 0) {
		result = paginas_libres.primera_libre;
		paginas_libres.primera_libre += (uint) 0x1000; //0x1000 es unapagina en memoria, sera asi??
		paginas_libres.cantidad --;
	} 
	return result;
}

void inicializar_dir_pirata(uint* cr3, uint fisicmem, uint elteam, uint tipo_pirata){
	page_directory_entry* pageDirectory = (page_directory_entry*) mmu_gimme_gimme_page_wachin();

	//inicializa pagedirectory sin entradas
	init_directory_table(pageDirectory);
	*cr3 = (uint) pageDirectory; //esto es asi directo?????????
	
	if(tipo_pirata == 0){
		mmu_mapear_pagina((uint) 0x400000, *cr3, (uint) 0x10000 + ( (uint) 0x2000 * elteam), (uint) 0x3);
	} else {
		mmu_mapear_pagina((uint) 0x400000, *cr3, (uint) 0x11000 + ( (uint) 0x2000 * elteam), (uint) 0x3);
	}

	mmu_mover_codigo_pirata(cr3, (uint*) fisicmem);
	
	if (elteam == JUGADOR_A) {
		mmu_mapear_pagina( (uint) 0x800000, *cr3, (uint) 0x500000, (uint) 0x03);

		mmu_mapear_pagina( (uint) 0x800000 + (uint) 0x1000 * 01, *cr3, (uint) 0x500000 + (uint) 0x1000 * 01, (uint) 0x03);
		mmu_mapear_pagina( (uint) 0x800000 + (uint) 0x1000 * 80, *cr3, (uint) 0x500000 + (uint) 0x1000 * 80, (uint) 0x03);
		mmu_mapear_pagina( (uint) 0x800000 + (uint) 0x1000 * 81, *cr3, (uint) 0x500000 + (uint) 0x1000 * 81, (uint) 0x03);
	breakpoint();
	} else {
		mmu_mapear_pagina( ((uint) 0x1520000-1), *cr3, ((uint) 0x121FFFF-1), (uint) 0x03);
		
		mmu_mapear_pagina( ((uint) 0x1520000-1) - (uint) 0x1000 * 01, *cr3, ((uint) 0x121FFFF-1) - (uint) 0x1000 * 01, (uint) 0x03);
		mmu_mapear_pagina( ((uint) 0x1520000-1) - (uint) 0x1000 * 80, *cr3, ((uint) 0x121FFFF-1) - (uint) 0x1000 * 80, (uint) 0x03);
		mmu_mapear_pagina( ((uint) 0x1520000-1) - (uint) 0x1000 * 81, *cr3, ((uint) 0x121FFFF-1) - (uint) 0x1000 * 81, (uint) 0x03);
	}	

}

void mmu_mover_codigo_pirata(uint* cr3, uint* destino){ //TODO: REVISAR
	page_selector pagina;

	uint pageDirectory = *cr3 & 0XFFFFF000;
	uint pageDirOffset, pageTableOffset;
	PDE_INDEX(0x400000, pageDirOffset);
	PTE_INDEX(0x400000, pageTableOffset);


	//recorre directorios
	uint*  pageTable = (uint*)  *( (uint*) (pageDirectory + pageDirOffset));
	uint** page		 = (uint**) *( (uint*) ((uint) (pageTable-0x3) + pageTableOffset));
	
	uint i;
	for(i=0; i<1024; i++){
		pagina.contenido[i] = *( (uint*) ((uint)page - 0x3) + (i * 4) ); //general protaccion, muere aca
	}
	
	mmu_mapear_pagina((uint) 0x400000, *cr3, (uint) destino, 0x3);

	pageTable = (uint*)  *( (uint*) (pageDirectory + pageDirOffset)); // limpio atributos
	page	  = (uint**) *( (uint*) ((uint) (pageTable-0x3) + pageTableOffset));

	breakpoint();
	for(i=0; i<1024; i++){
		*( (uint*) ((uint)page - 0x3 + (i * 4)) ) = pagina.contenido[i];	
	}
	breakpoint();
}


void mmu_mapear_pagina(uint virt, uint cr3, uint fisica, uint attrs){
	uint pageDirectory = cr3 & 0XFFFFF000;
	uint pageDirOffset, pageTableOffset;
	PDE_INDEX(virt, pageDirOffset);
	PTE_INDEX(virt, pageTableOffset);

	//recorre directorios
	page_directory_entry *directoryEntry = (page_directory_entry*) (pageDirectory + pageDirOffset);
	page_table_entry 	 *tableEntry;

	//revisa si existe la pagina
	if (directoryEntry->P == 0) { //preguntar por el bit de presente
		tableEntry = (page_table_entry*) mmu_gimme_gimme_page_wachin();
		set_directory_entry(directoryEntry, tableEntry);
		init_page_table(tableEntry);
	} else {
		uint dir = directoryEntry->dir_pagina_tabla;
		tableEntry = (page_table_entry*) dir; //Preguntar porque esto funciona
	}
	
	tableEntry += pageTableOffset;
	set_table_entry(tableEntry, fisica, attrs);
	
	tlbflush();
}

void mmu_unmapear_pagina(uint virt, uint cr3){
	//parsea offsets dentro de directorio de paginas
	uint pageDirOffset, pageTableOffset;
	uint pageDirectory = cr3 & 0XFFFFF000;
	PDE_INDEX(virt, pageDirOffset);
	PTE_INDEX(virt, pageTableOffset);

	//recorre directorios
	uint*  pageTable = (uint*)  *( (uint*) pageDirectory + pageDirOffset);
	uint** page	 	 = (uint**) *( (uint*) ((uint) pageTable + pageTableOffset));
	
	//mata pagina
	*page = NULL;
	tlbflush();
}

/* Direcciones fisicas de directorios y tablas de paginas del KERNEL */
/* -------------------------------------------------------------------------- */

//La funcion fue escrita previo al agregador de las estructuras page_----_entry, no se cambio en la reimplementacion
void mmu_inicializar_dir_kernel(){
	uint i,k;
	uint* pageDirectory = (uint*) KERNEL_PAGE_DIRECTORY;
	uint* pageTable 	= (uint*) KERNEL_PAGE_TABLE;

	init_table(pageDirectory);

	for(i=0; i<3; i++){
		*(pageDirectory + i) = ((uint) 0x28000 + (i * (uint)0x1000) ) + 0x3;
		uint j = (uint) 0x0;
		for(k=0; k < 1024 ; k++){
			*(pageTable + k) =  j + (uint) 0x3;
			j = j + (uint) 0x1000;
		}
		pageTable += (uint) 0x1000;
	}
}



void init_page_table(page_table_entry* table){
	uint i;
	for(i=0; i<1024; i++){
		(table + i)->P = 0;
	}
}

void init_directory_table(page_directory_entry* table){
	uint i;
	for(i=0; i<1024; i++){
		(table + i)->P = 0;
	}
} 

void set_directory_entry(page_directory_entry* dir, page_table_entry* table){
	dir->dir_pagina_tabla = (uint) table;
	dir->disp  = 0x0;
	dir->G 	   = 1;
	dir->PS    = 0;
	dir->disp0 = 0;
	dir->A 	   = 0;
	dir->PCD   = 0;
	dir->PWT   = 0;
	dir->US    = PG_USER;
	dir->RW    = PG_READ_WRITE;
	dir->P     = PG_PRESENT;
}

void set_table_entry(page_table_entry* table, uint fisic, uint attrs){
	table->dir_pagina_mem = fisic >> 12; //cuanto habia que recortar???
	table->disp  = 0; 
	table->G 	 = attrs & 256;
	table->PAT   = attrs & 128;
	table->D     = attrs & 64;
	table->A 	 = attrs & 32;
	table->PCD   = attrs & 16;
	table->PWT   = attrs & 8;
	table->US    = attrs & 4;
	table->RW    = attrs & 2;
	table->P     = attrs & 1;
}

void init_table(uint* table){
	int i=0;
	for (i = 0; i < 1024; i++){
		*(table + i) = 0x2;
	}
}