#include "Grid.h"
#include <iostream>
#include <fstream>

////////////////////////////////////////////////////////////////
// create a node
GridNode::GridNode(int nID, int row, int column, bool isC)
{
	this->clear = isC;

	this->rCoord = row;
	this->cCoord = column;

	this->entity = NULL;

	if (isC)
		this->contains = '.';
	else
		this->contains = 'B';
}

// default constructor
GridNode::GridNode()
{
	nodeID = -999;			// mark these as currently invalid
	this->clear = true;
	this->contains = '.';
	parent = NULL;
} 

////////////////////////////////////////////////////////////////
// destroy a node
GridNode::~GridNode()
{}  // doesn't contain any pointers, so it is just empty

////////////////////////////////////////////////////////////////
// set the node id
void 
GridNode::setID(int id)
{
	this->nodeID = id;
}

////////////////////////////////////////////////////////////////
// set the x coordinate
void 
GridNode::setRow(int r)
{
	this->rCoord = r;
}

////////////////////////////////////////////////////////////////
// set the y coordinate
void 
GridNode::setColumn(int c)
{
	this->cCoord = c;
}

////////////////////////////////////////////////////////////////
// get the x and y coordinate of the node
int 
GridNode::getRow()
{
	return rCoord;
}

int 
GridNode::getColumn()
{
	return cCoord;
}

// return the position of this node
Ogre::Vector3 
GridNode::getPosition(int rows, int cols)
{
	Ogre::Vector3 t;
	t.z = (rCoord * NODESIZE) - (rows * NODESIZE)/2.0 + (NODESIZE/2.0); 
	t.y = 10; 
	t.x = (cCoord * NODESIZE) - (cols * NODESIZE)/2.0 + (NODESIZE/2.0); 
	return t;
}

////////////////////////////////////////////////////////////////
// set the node as walkable
void 
GridNode::setClear()
{
	this->clear = true;
	this->contains = '.';
}

////////////////////////////////////////////////////////////////
// set the node as occupied
void 
GridNode::setOccupied()
{
	this->clear = false;
	this->contains = 'B';
}

////////////////////////////////////////////////////////////////
// is the node walkable
bool 
GridNode::isClear()
{
	return this->clear;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// create a grid
Grid::Grid(Ogre::SceneManager* mSceneMgr, int numRows, int numCols)
{
	this->mSceneMgr = mSceneMgr; 

	assert(numRows > 0 && numCols > 0);
	this->nRows = numRows;
	this->nCols = numCols;

	data.resize(numCols, GridRow(numRows));
		
	// put the coordinates in each node
	int count = 0;
	for (int i = 0; i < numRows; i++)
		for (int j = 0; j < numCols; j++)
		{
			GridNode *n = this->getNode(i,j);
			n->setRow(i);
			n->setColumn(j);
			n->setID(count);
			count++;
		}
}

/////////////////////////////////////////
// destroy a grid
Grid::~Grid(){};  														

////////////////////////////////////////////////////////////////
// get the node specified 
GridNode* 
Grid::getNode(int r, int c)
{
	//std::cout << r << "/" << nRows << " , " << c << "/" << nCols << std::endl;
	if (r >= nRows || c >= nCols || r < 0 || c < 0){
		return NULL;
	}

	return &this->data[c].data[r];
}

////////////////////////////////////////////////////////////////
// get adjacent nodes;
GridNode* 
Grid::getNorthNode(GridNode* n)
{
	if(n == NULL){
		return NULL;
	}

	GridNode* x = getNode(n->getRow()-1, n->getColumn());

	//return NULL if the node is out of bounds
	if(x==NULL){
		return NULL;
	}
	//return NULL if the space isn't clear
	if(!(x->isClear())){
		return NULL;
	}
	//return proper node otherwise
	return x;
}

GridNode* 
Grid::getSouthNode(GridNode* n)
{
	if(n == NULL){
		return NULL;
	}

	GridNode* x = getNode(n->getRow()+1, n->getColumn());

	//return NULL if the node is out of bounds
	if(x==NULL){
		return NULL;
	}
	//return NULL if the space isn't clear
	if(!(x->isClear())){
		return NULL;
	}
	//return proper node otherwise
	return x;
}

GridNode* 
Grid::getEastNode(GridNode* n)
{
	if(n == NULL){
		return NULL;
	}

	GridNode* x = getNode(n->getRow(), n->getColumn()+1);

	//return NULL if the node is out of bounds
	if(x==NULL){
		//std::cout << "x is null" << std::endl;
		return NULL;
	}
	//return NULL if the space isn't clear
	if(!(x->isClear())){
		return NULL;
	}
	//return proper node otherwise
	return x;
}

GridNode* 
Grid::getWestNode(GridNode* n)
{
	if(n == NULL){
		return NULL;
	}

	GridNode* x = getNode(n->getRow(), n->getColumn()-1);
	//return NULL if the node is out of bounds
	if(x==NULL){
		return NULL;
	}
	//return NULL if the space isn't clear
	if(!(x->isClear())){
		return NULL;
	}
	//return proper node otherwise
	return x;
}

GridNode* 
Grid::getNENode(GridNode* n)  
{
	//making sure a diagonal point is accessible form both sides
	if(getEastNode(getNorthNode(n))){
		return getNorthNode(getEastNode(n)); 
	}

	return NULL;
}

GridNode* 
Grid::getNWNode(GridNode* n) 
{
	//making sure a diagonal point is accessible form both sides
	if(getWestNode(getNorthNode(n))){
		return getNorthNode(getWestNode(n));
	}

	return NULL;
}

GridNode* 
Grid::getSENode(GridNode* n) 
{
	//making sure a diagonal point is accessible form both sides
	if(getEastNode(getSouthNode(n))){
		return getSouthNode(getEastNode(n));
	}

	return NULL;
}

GridNode* 
Grid::getSWNode(GridNode* n) 
{
	//making sure a diagonal point is accessible form both sides
	if(getWestNode(getSouthNode(n))){
		return getSouthNode(getWestNode(n));
	}

	return NULL;
}
////////////////////////////////////////////////////////////////
//get distance between between two nodes
int 
Grid::getDistance(GridNode* node1, GridNode* node2)
{
	//add distance between rows to Manhattan distance
	int manDist = abs(node1->getRow() - node2->getRow());
	//add distance between columns to Manhattan distance
	manDist += abs(node1->getColumn() - node2->getColumn());
	return manDist;
}

///////////////////////////////////////////////////////////////////////////////
// Print out the grid in ASCII
void 
Grid::printToFile()
{
	std::string path = __FILE__; //gets the current cpp file's path with the cpp file
	path = path.substr(0,1+path.find_last_of('\\')); //removes filename to leave path
	path+= "Grid.txt"; //if txt file is in the same directory as cpp file
	std::ofstream outFile;
	outFile.open(path);

	if (!outFile.is_open()) // oops. there was a problem opening the file
	{
		std::cout << "ERROR, FILE COULD NOT BE OPENED" << std::endl;	
		return;
	}

	for (int i = 0; i < nRows; i++)
	{
		for (int j = 0; j < nCols; j++)
		{
			outFile << this->getNode(i, j)->contains << " ";
		}
		outFile << std::endl;
	}
	outFile.close();
}

void // load and place a model in a certain location.
Grid::loadObject(std::string name, std::string filename, int row, int height, int col, float scale)
{
	using namespace Ogre;

	if (row >= nRows || col >= nCols || row < 0 || col < 0)
		return;

	Entity *ent = mSceneMgr->createEntity(name, filename);
    SceneNode *node = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,
        Ogre::Vector3(0.0f, 0.0f,  0.0f));
    node->attachObject(ent);
    node->setScale(scale, scale, scale);

	GridNode* gn = this->getNode(row, col);
	node->setPosition(getPosition(row, col)); 
	node->setPosition(getPosition(row, col).x, height, getPosition(row, col).z);
	gn->setOccupied();
	gn->entity = ent;
}

////////////////////////////////////////////////////////////////////////////
// Added this method and changed GridNode version to account for varying floor 
// plane dimensions. Assumes each grid is centered at the origin.
// It returns the center of each square. 
Ogre::Vector3 
Grid::getPosition(int r, int c)	
{
	Ogre::Vector3 t;
	t.z = (r * NODESIZE) - (this->nRows * NODESIZE)/2.0 + NODESIZE/2.0; 
	t.y = 10; 
	t.x = (c * NODESIZE) - (this->nCols * NODESIZE)/2.0 + NODESIZE/2.0; 
	return t;
}

void
Grid::eraseParents(){
	for(int y = 0; y < nCols; y++){
		for(int x = 0; x < nRows; x++){
			this->data[y].data[x].parent = NULL;
		}
	}
}

int Grid::getRowNum(){
	return nRows;
}

int Grid::getColNum(){
	return nCols;
}

//this is a interesting way to determine a win condition but i'll explain my logic:
//we are gonna assume that if we see no pallets (aka IDs of 2), then the board is empty therefore we won
//otherwise, the moment we see a ID == 2, that means there still exists at least one pallet and therefore, we havn't won yet
bool
Grid::isDone()
{
	for(int y = 0; y < nCols; y++){
		for(int x = 0; x < nRows; x++){
			if (this->data[y].data[x].getID() == 2)
				return false;
		}
	}
	return true;
}