#include "mpi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define min(a,b) ((a) < (b)) ? (a) : (b)

int MaxIntSize(val)
int val;
{
	int size = 1;     

	if (val < 0) {
		val = abs(val);
		size++;         
	}

	while (val > 9) {
		val = val / 10;
		size++;
	}

	return(size);
}

/*----------------------------------------------------------------------
// Kiem tra dau vao
//--------------------------------------------------------------------*/
int in_parameter(argc, argv, inf, outf)
int    argc;
char* argv[];
FILE** inf;
FILE** outf;
{
	int i;

	//printf("argc: %d \n", argc);
	printf("File dau vao: %s \n", argv[2]);
	printf("File dau ra: %s \n", argv[4]);
	//printf("USAGE: %s -input infile -output outfile\n", argv[0]);
	for (i = 0; i < argc; i++)
		if (_stricmp(argv[i], "-input") == 0)
			if (++i < argc) {
				if ((*inf = fopen(argv[i], "r")) == 0) {
					printf("Khong the mo file dau vao.\n");
					return (0);
				}
			}
			else {
				printf("Khong du tham so!");
				printf("Cach su dung: %s -input infile -output outfile\n", argv[0]);
				return (0);
			}
		else if (_stricmp(argv[i], "-output") == 0)
			if (++i < argc) {
				if ((*outf = fopen(argv[i], "w")) == 0) {
					printf("Khong the mo file dau ra.\n");
					return (0);
				}
			}
			else {
				printf("Khong du tham so!");
				printf("Cach su dung: %s -input infile -output outfile\n", argv[0]);
				return (0);
			}
	if (argc = 0)
		printf("Cach su dung: %s -input infile -output outfile\n", argv[0]);
	//printf("Cut2");

	return(1);
}


/*----------------------------------------------------------------------
// In bang ma tran dau ra.
//--------------------------------------------------------------------*/
int PrintMatrix(file, matrix, size)
FILE* file;
int* matrix;
int   size;
{
	int  i, j;
	int  maxval;
	int  padding;
	char formatstring[10];

	maxval = 0;
	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
			if (abs(maxval) < abs(matrix[i * size + j]))
				maxval = matrix[i * size + j];

	padding = MaxIntSize(maxval) + 1;

	sprintf(formatstring, " %%%dd", padding);

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++)
			fprintf(file, formatstring, matrix[i * size + j]);
		fprintf(file, "\n");
	}
	fprintf(file, "\n");
	fclose(file);
}

/*----------------------------------------------------------------------
// Doc file dau vao
//--------------------------------------------------------------------*/
int ReadFile(file, size, graph)
FILE* file;
int* size;
int** graph;
{
	int i, j;

	fscanf(file, "%d", size);

	*graph = (int*)malloc((*size) * (*size) * sizeof(int));

	for (i = 0; i < (*size); i++)
		for (j = 0; j < (*size); j++)
			fscanf(file, "%d", &(*graph)[i * (*size) + j]);

	fclose(file);
	return(1);
}

/*----------------------------------------------------------------------
// Chuong trinh chinh
//--------------------------------------------------------------------*/
int main(int argc, char** argv)
{
	int        i, j, k, np;
	int* graph = NULL;
	int* matran_trenbxl;
	int* kth_row;
	int        size;
	int        sodongmoibxl;
	int        my_rank, processors;
	FILE* in_file = NULL;
	FILE* out_file = NULL;
	MPI_Status status;
	double start, end;
	/*----------
	// Khoi tao  MPI
	//--------*/
	//printf("--- Khoi tao MPI\n");
	MPI_Init(&argc, &argv);
	start = MPI_Wtime();
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processors);

	//printf("Bug 1");
	if (my_rank == 0)
		if (!in_parameter(argc, argv, &in_file, &out_file)) {
			//printf("Exit 1");
			exit(1);
		}
	//printf("Bug 2");

	/*----------
	// Doc file dau vao.
	//--------*/
	if (my_rank == 0)
		if (!ReadFile(in_file, &size, &graph)) {
			//printf("program terminating\n");
			exit(1);
		}

	/*----------
	// Quang ba so dinh cua do thi (Broadcast data size).
	//--------*/
	MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	/*----------
	//  chia du lieu data partitions.
	//--------*/
	//if (size % processors != 0)
	np = size - processors * (size / processors);
	//printf("%d np",np);
	if (np == 0)
		sodongmoibxl = size / processors;
	else {
		for (i = np; i < processors - 1; i++)
			if (my_rank == 1)
				sodongmoibxl = size / processors;
		for (i = 0; i < np - 1; i++)
			if (my_rank == 0)
				sodongmoibxl = (size / processors) + 1;

	}

	//printf("Main 1");

	matran_trenbxl = (int*)malloc(size * sodongmoibxl * sizeof(int));
	kth_row = (int*)malloc(size * 1 * sizeof(int));
	//printf("Main 2");
	//neu bieu thuc trong ham assert() la true thi tiep tuc, else chuong trinh ket thuc
	assert(matran_trenbxl != NULL);

	/*----------
	//Chia data.
	//--------*/
	MPI_Scatter(graph, sodongmoibxl * size, MPI_INT, matran_trenbxl,
		sodongmoibxl * size, MPI_INT, 0, MPI_COMM_WORLD);


	/*----------
	// thuc hien tinh.
	//--------*/
	for (k = 0; k < size; k++) {

		/*----------
		// Quang ba dong thu k (Broadcast kth row).
		//--------*/
		if (my_rank == (k / sodongmoibxl))
			for (i = 0; i < size; i++)
				kth_row[i] = matran_trenbxl[(k % sodongmoibxl) * size + i];
		MPI_Bcast(kth_row, size, MPI_INT, (k / sodongmoibxl), MPI_COMM_WORLD);

		/*----------
		// tinh lai cac dong.
		//--------*/
		for (i = 0; (i < sodongmoibxl) && (i < size); i++) {
			for (j = 0; j < size; j++) {
				matran_trenbxl[i * size + j] = min(matran_trenbxl[i * size + j],
					matran_trenbxl[i * size + k] + kth_row[j]);
			}
		}
	}

	/*----------
	// Ket noi data.
	//--------*/
	MPI_Gather(matran_trenbxl, sodongmoibxl * size, MPI_INT, graph,
		sodongmoibxl * size, MPI_INT, 0, MPI_COMM_WORLD);

	/*----------
	// Print the output.
	//--------*/	
	if (my_rank == 0)
	{
		PrintMatrix(out_file, graph, size); end = MPI_Wtime(); printf("Thoi gian chay  %f giay\n", end - start);
	}

	/*----------
	// Thoat MPI.
	//--------*/
	MPI_Finalize();
}
