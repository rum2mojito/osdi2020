int
strcmp(char *a, char*b)
{
	while(*a != '\0') {
		if(*a != *b) return 0;
		a++;
		b++;
	}
	return 1;
}
