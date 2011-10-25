#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct	s_file
{
  unsigned int	len;
  unsigned char	*file;
  unsigned int	fcrt_offset;
  int		fcrt_len;
  unsigned char	fcrt_meta[4];
}		t_file;

void	quit(int status)
{
  printf("\nPress <enter> to continue\n");
  fgetc(stdin);
  exit(status);
}

void		load_nand(char *file, t_file *nand)
{
  size_t	len;
  FILE		*stream;
  struct stat	sb;

  if (stat(file, &sb) == -1)
    {
      perror("stat");
      quit(EXIT_FAILURE);
    }
  if (sb.st_size != 17301504)
    {
      fprintf(stdout, "The filesize is incorrect.\n");
      quit(EXIT_FAILURE);
    }
  nand->len = 0;
  printf("Allocating memory\n");
  nand->file = malloc(sb.st_size * sizeof(*(nand->file)));
  if ((stream = fopen(file, "rb")) == NULL)
    {
      perror("fopen");
      quit(EXIT_FAILURE);
    }
  printf("Load nand into memory\n");
  while ((len = fread(nand->file + nand->len, 1, 512, stream)) > 0)
    {
      nand->len += len;
    }
  if (nand->len != sb.st_size)
    {
      fprintf(stdout, "The entire file could not be read.\n");
      quit(EXIT_FAILURE);
    }
  fclose(stream);
}

int	get_fcrt_str(t_file *nand)
{
  int	offset = 0;
  char	*pattern = "fcrt.bin";
  int	plen = strlen(pattern);

  printf("Starting search for %s\n", pattern);
  while (nand->len - offset > plen)
    {
      if (strncmp(pattern, nand->file + offset, plen) == 0)
	{
	  return (offset + 16);
	}
      offset++;
    }
  return (-1);
}

unsigned int	read_nb(char *file, int offset, int len)
{
  unsigned int	nb = 0;
  int		i = 0;

  while (i < len)
    {
      nb = nb * 256 + (file[offset + i] & 255);
      ++i;
    }
  return (nb);
}

void	get_fcrt_info(t_file *nand, int offset)
{
  nand->fcrt_offset = read_nb(nand->file, offset, 8);
  printf("Data start block is at offset %#.4x\n", nand->fcrt_offset);
  offset += 8;
  nand->fcrt_len = read_nb(nand->file, offset, 4);
  offset += 4;
  printf("Fcrt lenght is %d\n", nand->fcrt_len);
  strncpy(nand->fcrt_meta, nand->file + offset, 4);
  printf("Meta is : %x %x %x %x\n",
	 nand->fcrt_meta[0],nand->fcrt_meta[1], nand->fcrt_meta[2], nand->fcrt_meta[3]);
}

void	write_fcrt(t_file *nand)
{
  FILE	*stream;
  int	offset;
  int	writed = 0;

  if ((stream = fopen("fcrt.bin", "wb+")) == NULL)
    {
      perror("fopen");
      quit(EXIT_FAILURE);
    }
  offset = nand->fcrt_offset * 16896;
  while (writed < nand->fcrt_len)
    {
      fwrite(nand->file + offset, 1, 0x200, stream);
      writed += 0x200;
      offset += 0x210;
    }
  fclose(stream);
  if ((stream = fopen("fcrt.bin.meta", "wb+")) == NULL)
    {
      perror("open");
      quit(EXIT_FAILURE);
    }
  fwrite(nand->fcrt_meta, 1, 4, stream);
  fclose(stream);
}

int		main(int argc, char *argv[])
{
  int		offset;
  t_file	nand;

  printf("---------------------------------------------------------------\n"
         "    fcrt eXtractor v0.03 by BestPig\n"
         "---------------------------------------------------------------\n");
  if (argc < 2)
    {
      printf("Usage:\n"
	     "    fcrt_extractor.exe nand.bin\n");
    }
  else
    {
      load_nand(argv[1], &nand);
      offset = get_fcrt_str(&nand);
      if (offset < 0)
        {
	  printf("No fcrt.bin found in this nand.\n");
	  quit(EXIT_FAILURE);
        }
      get_fcrt_info(&nand, offset);
      write_fcrt(&nand);
    }
  quit(EXIT_SUCCESS);
  return (0);
}
