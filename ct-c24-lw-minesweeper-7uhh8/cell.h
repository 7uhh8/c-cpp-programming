#ifndef CELL_H
#define CELL_H

struct Cell
{
	int row;
	int col;
	bool mine;
	bool revealed;
	bool flagged;

	Cell(int r = 0, int c = 0);
};

#endif	  // CELL_H
