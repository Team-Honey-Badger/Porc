#include "Agent.h"
#include "Grid.h"

Agent::Agent(Ogre::SceneManager* SceneManager, std::string name, std::string filename, float height, float scale, Grid *grid, char type)
{
	using namespace Ogre;

	//attach the grid
	this->grid = grid;

	//initialize GridNodes
	selfNode = NULL;
	startNode = new GridNode(-1, 4, 1, true);
	goalNode = new GridNode(-1, 4, 8, true);
	toggle = true;
	this->agentType = type;

	if(agentType == 'g'){
		//make a list of intersections
		GridNode *temp = grid->getNode(0,0);
		goalNode = temp;
	}

	mSceneMgr = SceneManager; // keep a pointer to where this agent will be

	if (mSceneMgr == NULL)
	{
		std::cout << "ERROR: No valid scene manager in Agent constructor" << std::endl;
		return;
	}

	this->height = height;
	this->scale = scale;

	mBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(); // create a new scene node
	mBodyEntity = mSceneMgr->createEntity(name, filename); // load the model
	mBodyNode->attachObject(mBodyEntity);	// attach the model to the scene node

	mBodyNode->translate(0,height,0); // make the Ogre stand on the plane (almost)
	mBodyNode->scale(scale,scale,scale); // Scale the figure

	setupAnimations();  // load the animation for this character

	// configure walking parameters
	mWalkSpeed = 35.0f;	
	mDirection = Ogre::Vector3::ZERO;

	//starting position isn't set yet
	isFirstPosSet = false;
	
}

Agent::~Agent(){
	// mSceneMgr->destroySceneNode(mBodyNode); // Note that OGRE does not recommend doing this. It prefers to use clear scene
	// mSceneMgr->destroyEntity(mBodyEntity);
}

void
Agent::setSelfNode(int r, int c){
	selfNode = grid->getNode(r,c);
	selfNode->parent = NULL;
}

void
Agent::setStartNode(int r, int c){
	delete startNode; //free the default node
	startNode = grid->getNode(r,c);
	startNode->parent = NULL;
}

void
Agent::setGoalNode(int r, int c){
	delete goalNode; //free the default node
	goalNode = grid->getNode(r,c);
	goalNode->parent = NULL;
}

void 
Agent::setPosition(float x, float y, float z)
{
	this->mBodyNode->setPosition(x, y, z);

	//set initial position
	if(!isFirstPosSet){
		mWalkList.push_back(Ogre::Vector3(x, y, z));
		isFirstPosSet = true;
	}
}

void
Agent::setOrientation(int orientation)
{
	this->orientation = orientation;
}

int
Agent::getOrientation()
{
	return this->orientation;
}

// update is called at every frame from GameApplication::addTime
void
Agent::update(Ogre::Real deltaTime) 
{
	this->updateAnimations(deltaTime);	// Update animation playback
	moveTo();							// Find out where to go
	this->updateLocomote(deltaTime);	// Update Locomotion
}


void 
Agent::setupAnimations()
{
	this->mTimer = 0;	// Start from the beginning
	this->mVerticalVelocity = 0;	// Not jumping

	// this is very important due to the nature of the exported animations
	mBodyEntity->getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);

	// Name of the animations for this character
	Ogre::String animNames[] =
		{"IdleBase", "IdleTop", "RunBase", "RunTop", "HandsClosed", "HandsRelaxed", "DrawSwords",
		"SliceVertical", "SliceHorizontal", "Dance", "JumpStart", "JumpLoop", "JumpEnd"};

	// populate our animation list
	for (int i = 0; i < 13; i++)
	{
		mAnims[i] = mBodyEntity->getAnimationState(animNames[i]);
		mAnims[i]->setLoop(true);
		mFadingIn[i] = false;
		mFadingOut[i] = false;
	}

	// start off in the idle state (top and bottom together)
	setBaseAnimation(ANIM_IDLE_BASE);
	setTopAnimation(ANIM_IDLE_TOP);

	// relax the hands since we're not holding anything
	mAnims[ANIM_HANDS_RELAXED]->setEnabled(true);
}

void 
Agent::setBaseAnimation(AnimID id, bool reset)
{
	if (mBaseAnimID >= 0 && mBaseAnimID < 13)
	{
		// if we have an old animation, fade it out
		mFadingIn[mBaseAnimID] = false;
		mFadingOut[mBaseAnimID] = true;
	}

	mBaseAnimID = id; 

	if (id != ANIM_NONE)
	{
		// if we have a new animation, enable it and fade it in
		mAnims[id]->setEnabled(true);
		mAnims[id]->setWeight(0);
		mFadingOut[id] = false;
		mFadingIn[id] = true;
		if (reset) mAnims[id]->setTimePosition(0);
	}
}
	
void Agent::setTopAnimation(AnimID id, bool reset)
{
	if (mTopAnimID >= 0 && mTopAnimID < 13)
	{
		// if we have an old animation, fade it out
		mFadingIn[mTopAnimID] = false;
		mFadingOut[mTopAnimID] = true;
	}

	mTopAnimID = id;

	if (id != ANIM_NONE)
	{
		// if we have a new animation, enable it and fade it in
		mAnims[id]->setEnabled(true);
		mAnims[id]->setWeight(0);
		mFadingOut[id] = false;
		mFadingIn[id] = true;
		if (reset) mAnims[id]->setTimePosition(0);
	}
}

void
Agent::updateBody(Ogre::Real deltaTime)
{

}

void 
Agent::updateAnimations(Ogre::Real deltaTime)
{
	using namespace Ogre;

	Real baseAnimSpeed = 1;
	Real topAnimSpeed = 1;

	mTimer += deltaTime; // how much time has passed since the last update
	
	// increment the current base and top animation times
	if (mBaseAnimID != ANIM_NONE) mAnims[mBaseAnimID]->addTime(deltaTime * baseAnimSpeed);
	if (mTopAnimID != ANIM_NONE) mAnims[mTopAnimID]->addTime(deltaTime * topAnimSpeed);

	// apply smooth transitioning between our animations
	fadeAnimations(deltaTime);
}

void 
Agent::fadeAnimations(Ogre::Real deltaTime)
{
	using namespace Ogre;

	for (int i = 0; i < 13; i++)
	{
		if (mFadingIn[i])
		{
			// slowly fade this animation in until it has full weight
			Real newWeight = mAnims[i]->getWeight() + deltaTime * 7.5f; //ANIM_FADE_SPEED;
			mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
			if (newWeight >= 1) mFadingIn[i] = false;
		}
		else if (mFadingOut[i])
		{
			// slowly fade this animation out until it has no weight, and then disable it
			Real newWeight = mAnims[i]->getWeight() - deltaTime * 7.5f; //ANIM_FADE_SPEED;
			mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
			if (newWeight <= 0)
			{
				mAnims[i]->setEnabled(false);
				mFadingOut[i] = false;
			}
		}
	}
}

bool 
Agent::nextLocation()
{
	//check if there is another location
	return !mWalkList.empty();
}

void 
Agent::updateLocomote(Ogre::Real deltaTime)
{
	if (mDirection == Ogre::Vector3::ZERO) 
    {
        if (nextLocation()) 
        {
            //set to running animation
			if(mBaseAnimID != ANIM_RUN_BASE){ // stay in running if already in running (avoids T-pose issue)
				setBaseAnimation(ANIM_RUN_BASE);
				setTopAnimation(ANIM_RUN_TOP);
			}

			//get next position
			mDestination = mWalkList.front();  // this gets the front of the deque
			mWalkList.pop_front();             // this removes the front of the deque

			//set movement data
			mDirection = mDestination - mBodyNode->getPosition();
			mDistance = mDirection.normalise();

			//set rotation
			//different elevations mess up the rotation
			Ogre::Vector3 src = mBodyNode->getOrientation() * Ogre::Vector3::UNIT_Z;
			if ((1.0f + src.dotProduct(mDirection)) < 0.0001f) 
			{
				mBodyNode->yaw(Ogre::Degree(180));
			}
			else
			{
				Ogre::Quaternion quat = src.getRotationTo(mDirection);
				mBodyNode->rotate(quat);
			}
        }
		else
		{
			//set to idle animation
			if(mBaseAnimID != ANIM_IDLE_BASE){
				setBaseAnimation(ANIM_IDLE_BASE);
				setTopAnimation(ANIM_IDLE_TOP);
			}
		}
    }
	else
	{
		//move model
		Ogre::Real move = mWalkSpeed * deltaTime;
		mBodyNode->translate(mDirection * move);
		mDistance -= move;
	}

	if (mDistance <= 0.0f)
    {
		//stop movement after reaching destination
        mBodyNode->setPosition(mDestination);
        mDirection = Ogre::Vector3::ZERO;
	}
}

void
Agent::moveTo(){
	using namespace std;
	
	//quit if previous A* is in progress
	if(nextLocation()){
		return;
	}
	if(agentType == 'c'){ //if player
		switch(this->orientation)
		{
		case 1:
			if (grid->getNorthNode(this->selfNode) != NULL)
			{
				GridNode *northNode = grid->getNorthNode(this->selfNode);
				mWalkList.push_back(grid->getPosition(northNode->getRow(), northNode->getColumn()));
				selfNode = northNode;
			}
			break;
		case 2:
			if (grid->getSouthNode(this->selfNode) != NULL)
			{
				GridNode *southNode = grid->getSouthNode(this->selfNode);
				mWalkList.push_back(grid->getPosition(southNode->getRow(), southNode->getColumn()));
				selfNode = southNode;
			}
			break;
		case 3:
			if (grid->getEastNode(this->selfNode) != NULL)
			{
				GridNode *eastNode = grid->getEastNode(this->selfNode);
				mWalkList.push_back(grid->getPosition(eastNode->getRow(), eastNode->getColumn()));
				selfNode = eastNode;
			}
			break;
		case 4:
			if (grid->getWestNode(this->selfNode) != NULL)
			{
				GridNode *westNode = grid->getWestNode(this->selfNode);
				mWalkList.push_back(grid->getPosition(westNode->getRow(), westNode->getColumn()));
				selfNode = westNode;
			}
			break;
		}
	}
	if(agentType == 'g'){ //if ghost
		//set start and goal
		GridNode *current;
		GridNode *goal;

		if( goalNode->getRow() == selfNode->getRow() && goalNode->getColumn() == selfNode->getColumn() ){ // has goal been reached?
			//set new goal
			current = selfNode;
			goal = selfNode;

			//return;
		}
		else{ // no need to find a new goal
			return;
		}


		/*if(toggle){
			goal = goalNode;
		}
		else{
			goal = startNode;
		}
		toggle = !toggle;*/
		
		list<GridNode*> open;
		list<GridNode*> closed;

		//add initial position to open list
		open.push_back(selfNode);

		//values for A*
		int F, G, H; //A* values
		GridNode *bestN; //best node
		int bestF;	//best F value
		bool inClosed, inOpen; //bools checking if node is already in a list
		list<GridNode*> temp; //holds all neighbor nodes

		//find path
		while(true){

			//add new and clear adjacent nodes to open list
			temp.clear(); //clear previous neighbors
			temp.push_front(grid->getEastNode(current));
			temp.push_front(grid->getNENode(current));
			temp.push_front(grid->getNorthNode(current));
			temp.push_front(grid->getNWNode(current));
			temp.push_front(grid->getWestNode(current));
			temp.push_front(grid->getSWNode(current));
			temp.push_front(grid->getSouthNode(current));
			temp.push_front(grid->getSENode(current));
			for(list<GridNode*>::iterator j = temp.begin(); j != temp.end(); j++){
				if((*j) != NULL){
					inClosed = false;
					for(list<GridNode*>::iterator i = closed.begin(); i != closed.end(); i++){
						if((*i) == (*j)){
							inClosed = true;
							break;
						}
					}
					inOpen = false;
					for(list<GridNode*>::iterator i = open.begin(); i != open.end(); i++){
						if((*i) == (*j)){
							inOpen = true;
							break;
						}
					}
					if(!inClosed && !inOpen){
						open.push_front((*j));
					}
				}
			}
		
			//find current's best neighbor
			bestN = NULL; //best node
			bestF = 99999999;	//best F value
		
			for(list<GridNode*>::iterator i = open.begin(); i != open.end(); i++){
				//find distance cost
				H = 10 * grid->getDistance((*i), goal);
				//find movement cost
				G = 10 * grid->getDistance(current, (*i));
				if( G = 20 ){
					G = 14; //compensate for diagonals
				}
				//add up total
				F = G + H;

				//compare F values and if the current iteration beats the best, set it as the new best
				if( F < bestF ){
					bestF = F;
					bestN = (*i);
				}
			}
		
			//if a neighbor was found
			if(bestN != NULL){

				//iterate closed list backwards
				for(list<GridNode*>::reverse_iterator i = closed.rbegin(); i != closed.rend(); i++){
					if((*i) != current){
						//if one of the parent nodes is adjacent to the selected node
						if( ( abs( (*i)->getRow()-bestN->getRow() ) <= 1) && ( abs( (*i)->getColumn()-bestN->getColumn() ) <= 1) ){
							//bring current back to it and restart the closed list from here
							current = (*i);
							break;
						}
					}
				}

				//move current to the best neighbor
				bestN->parent = current;
				current = bestN;

				//remove the selected node from open list
				open.remove(current);
				//add it to closed list
				closed.push_front(current);
			}
			//else return to previous closed list node
			else{
				closed.remove(current);
				GridNode *temp = current;
				current = current->parent;
				temp->parent = NULL;
			}

			//break if it reached the goal
			if( current->getRow() == goal->getRow() && current->getColumn() == goal->getColumn() ){
			
				int counter = 0;
			
				//set up counter to count backwards
				GridNode *countDown = current;
				while(countDown != selfNode){
					counter++;
					countDown = countDown->parent;
				}

				//backtrack the path to the goal into the walklist
				while( current != selfNode ){
					mWalkList.push_front(grid->getPosition(current->getRow(), current->getColumn()));

					//traverse to parent
					current = current->parent;

				}

				//set agents initial position to the goal position it just reached
				setSelfNode(goal->getRow(), goal->getColumn());
				break;
			}

			//break if it runs out of nodes
			if( open.empty() ){
				break;
			}
		}
	}
}