#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

int main(int argc, char * argv[]) {
	int rank;
	int nProcesses;

	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Request request;


	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
	
	unsigned char *unaltered_image, *altered_image, *received_slice,
	*slice_to_send, *leftover_slice;
	int *meta_data, slice_height, slice_remainder;
	
	meta_data = malloc(5 * sizeof(int));
	
	
	
	// READ
	if (rank == 0)
	{
		FILE *input;
		input = fopen(argv[1], "r");
		if (input == NULL)
		{
			printf("Cannot open file\n");
			exit(0);
		}
		fscanf(input, "P%d\n", &meta_data[0]);
		if(meta_data[0] == 5)
			meta_data[4] = 1;
		if(meta_data[0] == 6)
			meta_data[4] = 3;
		int i, j;
		char bla;
		for (i =0; i < 46; i++)
		{
			bla = fgetc(input);
		}
		fscanf(input, "%d %d\n%d\n", &meta_data[1], &meta_data[2], &meta_data[3]);
		unaltered_image = malloc(meta_data[4] * (meta_data[2] + 2) * (meta_data[1] + 2) * sizeof(unsigned char));
		for (i = 0; i < (meta_data[4] * meta_data[2] + 2) * (meta_data[1] + 2); i++)
		{
			unaltered_image[i] = 0;
		}
		for (i = 1; i < meta_data[2] + 1; i++)
		{
			fread(unaltered_image + (meta_data[4] * ((i * (meta_data[1] + 2)) + 1)), 1, meta_data[1] * meta_data[4], input);
		}
		fclose(input);
	}

	// SHARE META DATA
	MPI_Bcast(meta_data, 5, MPI_INT, 0, MPI_COMM_WORLD);
	
	slice_height = meta_data[2] / nProcesses;
	slice_remainder = meta_data[2] % nProcesses;

	// UNALTERED BLOCK RECEIVED
	received_slice = malloc(meta_data[4] * (meta_data[1] + 2) * (slice_height + 2) * sizeof(unsigned char));
	// ALTERED BLOCK TO SEND
	slice_to_send = malloc(meta_data[4] * (meta_data[1] + 2) * slice_height * sizeof(unsigned char));
	// LEFTOVER BLOCK - PROCESSED BY 0
	leftover_slice = malloc(meta_data[4] * (meta_data[1] + 2) * slice_remainder * sizeof(unsigned char));
	
	for(int j = 0; j < meta_data[4] * (meta_data[1] + 2) * slice_height; j++)
	{
		slice_to_send[j] = 0;
	}
	for(int j = 0; j < meta_data[4] * (meta_data[1] + 2) * slice_remainder; j++)
	{
		leftover_slice[j] = 0;
	}
	
	// SET UP CLOCK
	clock_t clk;
	clk = clock();

	// LOOP FOR EACH FILTER
	for(int filter_no = 3; filter_no < argc; filter_no++)
	{
		// SHARE PIXEL BLOCKS
		if(rank == 0)
		{
			for(int i = 0; i < meta_data[4] * (slice_height + 2) * (meta_data[1] + 2); i++)
			{
				received_slice[i] = unaltered_image[i];
			}
			for(int i = 1; i < nProcesses; i++)
			{
				MPI_Send(unaltered_image + meta_data[4] * (meta_data[1] + 2) * (i * slice_height), meta_data[4] * (slice_height + 2) * (meta_data[1] + 2), MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
			}
		}
		if(rank != 0)
		{
				MPI_Recv(received_slice, meta_data[4] * (slice_height + 2) * (meta_data[1] + 2), MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &status);
		}
		
		// APPLY FILTER FOR EACH COLOUR
		float aux;
		int temp;
		for(int c = 0; c < meta_data[4]; c++)
		{
			if(strcmp(argv[filter_no], "smooth") == 0)
			{
				for(int i = 1; i < slice_height + 1; i++)
				{
					for(int j = 1; j < meta_data[1] + 1; j++)
					{
						aux = 0.f;
						aux += (1.f / 9.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (1.f / 9.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						temp = (int) aux;
						if(temp < 0)
							temp = 0;
						if(temp > meta_data[3])
							temp = meta_data[3];
						slice_to_send[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
					}
				}
				if(rank == 0)
				{
					int remainder_offset;
					remainder_offset = (meta_data[1] + 2) * meta_data[4] * slice_height * nProcesses;
					for(int i = 1; i < slice_remainder + 1; i++)
					{
						for(int j = 1; j < meta_data[1] + 1; j++)
						{
							aux = 0.f;
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (1.f / 9.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							temp = (int) aux;
							if(temp < 0)
								temp = 0;
							if(temp > meta_data[3])
								temp = meta_data[3];
							leftover_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						}
					}
				}
			}
			else if(strcmp(argv[filter_no], "blur") == 0)
			{
				for(int i = 1; i < slice_height + 1; i++)
				{
					for(int j = 1; j < meta_data[1] + 1; j++)
					{
						aux = 0.f;
						aux += (1.f / 16.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (2.f / 16.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (1.f / 16.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux += (2.f / 16.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (4.f / 16.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (2.f / 16.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux += (1.f / 16.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (2.f / 16.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += (1.f / 16.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						temp = (int) aux;
						
						if(temp > meta_data[3])
							temp = meta_data[3];
						slice_to_send[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						
					}
				}
				if(rank == 0)
				{
					int remainder_offset;
					remainder_offset = (meta_data[1] + 2) * meta_data[4] * slice_height * nProcesses;
					for(int i = 1; i < slice_remainder + 1; i++)
					{
						for(int j = 1; j < meta_data[1] + 1; j++)
						{
							aux = 0.f;
							aux += (1.f / 16.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (2.f / 16.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (1.f / 16.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux += (2.f / 16.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (4.f / 16.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (2.f / 16.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux += (1.f / 16.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (2.f / 16.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += (1.f / 16.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							temp = (int) aux;
							if(temp < 0)
								temp = 0;
							if(temp > meta_data[3])
								temp = meta_data[3];
							leftover_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						}
					}
				}
			}
			if(strcmp(argv[filter_no], "sharpen") == 0)
			{
				for(int i = 1; i < slice_height + 1; i++)
				{
					for(int j = 1; j < meta_data[1] + 1; j++)
					{
						aux = 0.f;
						aux -= (2.f / 3.f) * (float) received_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux -= (2.f / 3.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += (11.f / 3.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux -= (2.f / 3.f) * (float) received_slice[((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux -= (2.f / 3.f) * (float) received_slice[((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						temp = (int) aux;
						if(temp < 0)
							temp = 0;
						if(temp > meta_data[3])
							temp = meta_data[3];
						slice_to_send[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
					}
				}
				if(rank == 0)
				{
					int remainder_offset;
					remainder_offset = (meta_data[1] + 2) * meta_data[4] * slice_height * nProcesses;
					for(int i = 1; i < slice_remainder + 1; i++)
					{
						for(int j = 1; j < meta_data[1] + 1; j++)
						{
							aux = 0.f;
							aux -= (2.f / 3.f) * (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux -= (2.f / 3.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += (11.f / 3.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux -= (2.f / 3.f) * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux -= (2.f / 3.f) * (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							temp = (int) aux;
							if(temp < 0)
								temp = 0;
							if(temp > meta_data[3])
								temp = meta_data[3];
							leftover_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						}
					}
				}
			}
			if(strcmp(argv[filter_no], "mean") == 0)
			{
				for(int i = 1; i < slice_height + 1; i++)
				{
					for(int j = 1; j < meta_data[1] + 1; j++)
					{
						aux = 0.f;
						aux -= (float) received_slice[((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux -= (float) received_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux -= (float) received_slice[((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux -= (float) received_slice[((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux += 9.f * (float) received_slice[((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux -= (float) received_slice[((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						aux -= (float) received_slice[((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
						aux -= (float) received_slice[((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux -= (float) received_slice[((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
						temp = (int) aux;
						if(temp < 0)
							temp = 0;
						if(temp > meta_data[3])
							temp = meta_data[3];
						slice_to_send[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
					}
				}
				if(rank == 0)
				{
					int remainder_offset;
					remainder_offset = (meta_data[1] + 2) * meta_data[4] * slice_height * nProcesses;
					for(int i = 1; i < slice_remainder + 1; i++)
					{
						for(int j = 1; j < meta_data[1] + 1; j++)
						{
							aux = 0.f;
							aux -= (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux += 9.f * (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j - 1) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux -= (float) unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j + 1) * meta_data[4] + c];
							temp = (int) aux;
							if(temp < 0)
								temp = 0;
							if(temp > meta_data[3])
								temp = meta_data[3];
							leftover_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						}
					}
				}
			}
			if(strcmp(argv[filter_no], "emboss") == 0)
			{
				for(int i = 1; i < slice_height + 1; i++)
				{
					for(int j = 1; j < meta_data[1] + 1; j++)
					{
						aux = 0.0f;
						aux -= received_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						aux += received_slice[((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
						temp = (int) aux;
						if(temp < 0)
							temp = 0;
						if(temp > meta_data[3])
							temp = meta_data[3];
						slice_to_send[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
					}
				}
				if(rank == 0)
				{
					int remainder_offset;
					remainder_offset = (meta_data[1] + 2) * meta_data[4] * slice_height * nProcesses;
					for(int i = 1; i < slice_remainder + 1; i++)
					{
						for(int j = 1; j < meta_data[1] + 1; j++)
						{
							aux = 0.0f;
							aux -= unaltered_image[remainder_offset + ((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							aux += unaltered_image[remainder_offset + ((i + 1) * (meta_data[1] + 2) + j) * meta_data[4] + c];
							temp = (int) aux;
							if(temp < 0)
								temp = 0;
							if(temp > meta_data[3])
								temp = meta_data[3];
							leftover_slice[((i - 1) * (meta_data[1] + 2) + j) * meta_data[4] + c] = (unsigned char) temp;
						}
					}
				}
			}
		}

		// GATHER PIXEL BLOCKS
		if(rank == 0)
		{
			for(int i = 0; i < slice_height * (meta_data[1] + 2) * meta_data[4]; i++)
			{
				unaltered_image[i + (meta_data[1] + 2) * meta_data[4]] = slice_to_send[i];
			}
			for(int i = (meta_data[1] + 2) * (slice_height * nProcesses + 1) * meta_data[4]; i < (meta_data[1] + 2) * (meta_data[2] + 1) * meta_data[4]; i++)
			{
				unaltered_image[i] = leftover_slice[i - (meta_data[1] + 2) * (slice_height * nProcesses + 1) * meta_data[4]];
			}
			for(int i = 1; i < nProcesses; i++)
			{
				MPI_Recv(unaltered_image + (meta_data[1] + 2) * meta_data[4] * (i * slice_height + 1), slice_height * (meta_data[1] + 2) * meta_data[4], MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &status);
			}
		}
		if(rank != 0)
		{
			MPI_Send(slice_to_send, slice_height * (meta_data[1] + 2) * meta_data[4], MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
		}
	}

	// GET TIME
	clk = clock() - clk;
	if (rank == 0)
		printf("%f\n", ((double)clk)/CLOCKS_PER_SEC);

	// WRITE
	if (rank == 0)
	{
		FILE *output;
		output = fopen(argv[2], "w");
		if (output == NULL)
		{
			printf("Cannot open file\n");
			exit(0);
		}
		int i, j;
		fprintf(output, "P%d\n", meta_data[0]);
		fprintf(output, "%d %d\n%d\n", meta_data[1], meta_data[2], meta_data[3]);
		for (i = 1; i < meta_data[2] + 1; i++)
		{
			fwrite(unaltered_image + ((i * (meta_data[1] + 2)) + 1) * meta_data[4], 1, meta_data[1] * meta_data[4], output);
		}
		fclose(output);
	}
	
	MPI_Finalize();
	return 0;
}