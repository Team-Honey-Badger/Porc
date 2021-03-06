#include "Agent.h"
#include "Grid.h"

Agent::Agent(Ogre::SceneManager* SceneManager, std::string name, std::string filename, float height, float scale, Grid *grid, char type)
{
	using namespace Ogre;

	//attach the grid
	this->grid = grid;

	//initialize GridNodes
	selfNode = NULL;
	startNode = new GridNode(-1, 0, 0, true);
	goalNode = new GridNode(-1, 0, 0, true);
	toggle = false;
	this->agentType = type;
	orientation = 0;
	reset = false;
	doneWithLevel = false;
	pauseTimer = 0.0;

	//identify player and give him 3 lives
	if(agentType == 'c'){
		lives = 3;
	}
	else{ //else the agent is a ghost or a pellet
		lives = -1;
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

	//default to non-motion
	defaultOrientation = 0;
}

Agent::~Agent(){
	//use clear scene
}
//setter and getter for selfNode
void
Agent::setSelfNode(int r, int c){
	selfNode = grid->getNode(r,c);
	//selfNode->parent = NULL;
}

GridNode*
Agent::getSelfNode(){
	return selfNode;
}

void
Agent::setStartNode(int r, int c){
	delete startNode; //free the default node
	startNode = grid->getNode(r,c);
	//startNode->parent = NULL;
}

void
Agent::setGoalNode(){
	std::random_shuffle(intersections.begin(), intersections.end());	//shuffle list of destinations
	goalNode = intersections.at(0);										//select first (still random)
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

//getter and setters for Orientation
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

void 
Agent::setPlayer(Agent *player){
	this->player = player;
}

char
Agent::getAgentType()
{
	return this->agentType;
}

int
Agent::getLives()
{
	return this->lives;
}

// update is called at every frame from GameApplication::addTime
void
Agent::update(Ogre::Real deltaTime) 
{
	this->updateAnimations(deltaTime);		// Update animation playback	

	if(player) //if the current agent (this) is a player
	{
		if(player->pauseTimer > 0){				//while paused
			resetPositions(deltaTime);			//reposition ghosts and the player
		}
		else{
			collide(deltaTime);					// Check if Ghosts collide with player
			this->updateLocomote(deltaTime);	// Update Locomotion
			moveTo();							// Find out where to go
		}
	}
}


void 
Agent::setupAnimations()
{
	this->mTimer = 0;	// Start from the beginning
	this->mVerticalVelocity = 0;	// Not jumping

	if(agentType == 'c' || agentType == 'g'){

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

	//color the agents!
	if(agentType == 'g'){
		mBodyEntity->setMaterialName("Examples/Flare"); //looks like an actual ghost!
	}
	else if(agentType == 'c'){
		mBodyEntity->setMaterialName("Examples/Hilite/Yellow"); //pacman is yellow
	}
	else if(agentType == 'x'){
		mBodyEntity->setMaterialName("Examples/MorningSkyBox"); //looks right...
	}
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

	if(agentType == 'c' || agentType == 'g'){
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

	if(agentType == 'c' || agentType == 'g'){

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
}

void 
Agent::updateAnimations(Ogre::Real deltaTime)
{
	using namespace Ogre;

	if(agentType == 'c' || agentType == 'g'){

		Real baseAnimSpeed = 1;
		Real topAnimSpeed = 1;

		mTimer += deltaTime; // how much time has passed since the last update
	
		// increment the current base and top animation times
		if (mBaseAnimID != ANIM_NONE) mAnims[mBaseAnimID]->addTime(deltaTime * baseAnimSpeed);
		if (mTopAnimID != ANIM_NONE) mAnims[mTopAnimID]->addTime(deltaTime * topAnimSpeed);

		// apply smooth transitioning between our animations
		fadeAnimations(deltaTime);
	}
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
	//teleportation
	if(agentType == 'c' || agentType == 'g'){						//player and ghosts can teleport
		if(!nextLocation())											//wait till they come to a full stop before teleporting	
		{
			//left boundry to right
			if(selfNode->getColumn() == 0){							//teleporter enterance
				int y = 18, x = selfNode->getRow();					//teleporter exit
				mBodyNode->setPosition(grid->getPosition(x,y));		//move everything over
				mDestination = grid->getPosition(x,y);				//set destination to exit
				mWalkList.clear();									//clear any pathfinding
				mDistance = 0;										//clear distance since already here
				//mWalkList.push_front(grid->getPosition(x,y));		//walk here (just in case)
				selfNode = grid->getNode(x,y);						//tell agent where it is
				goalNode = grid->getNode(x,y);						//make agent want to be here
			}
			//right boundry to left
			else if(selfNode->getColumn() == 18){
				int y = 0, x = selfNode->getRow();
				mBodyNode->setPosition(grid->getPosition(x,y));
				mDestination = grid->getPosition(x,y);
				mWalkList.clear();
				mDistance = 0;
				//mWalkList.push_front(grid->getPosition(x,y));
				selfNode = grid->getNode(x,y);
				goalNode = grid->getNode(x,y);
			}
			//top boundry to bottom
			else if(selfNode->getRow() == 0){
				int x = 18, y = selfNode->getColumn();
				mBodyNode->setPosition(grid->getPosition(x,y));
				mDestination = grid->getPosition(x,y);
				mWalkList.clear();
				mDistance = 0;
				//mWalkList.push_front(grid->getPosition(x,y));
				selfNode = grid->getNode(x,y);
				goalNode = grid->getNode(x,y);
			}
			//bottom boundry to top
			else if(selfNode->getRow() == 18){
				int x = 0, y = selfNode->getColumn();
				mBodyNode->setPosition(grid->getPosition(x,y));
				mDestination = grid->getPosition(x,y);
				mWalkList.clear();
				mDistance = 0;
				//mWalkList.push_front(grid->getPosition(x,y));
				selfNode = grid->getNode(x,y);
				goalNode = grid->getNode(x,y);
			}
		}
	}

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

	//BARREL
	if(agentType == 'x'){
		if(selfNode->getID() == 3){
			mBodyNode->setVisible(false);

			if (grid->isDone())					//check if there are no more barrels
			{
				player->doneWithLevel = true;	//declare the level finished after the last barrel is collected
			}

		}
	}

	//PLAYER
	if(agentType == 'c'){

		selfNode->setID(3);	//note that you walked here

		if(mDirection == Ogre::Vector3::ZERO){ // only pick another location when not in motion

			//Here is where I use Orientation to control player movement, using a simple switch statement, i am able to easily break up player movement into four different cases
			switch(orientation)
			{
			case 1:
				if(grid->getNorthNode(selfNode)){
					defaultOrientation = orientation;
				}
				break;
			case 2:
				if(grid->getSouthNode(selfNode)){
					defaultOrientation = orientation;
				}
				break;
			case 3:
				if(grid->getEastNode(selfNode)){
					defaultOrientation = orientation;
				}
				break;
			case 4:
				if(grid->getWestNode(selfNode)){
					defaultOrientation = orientation;
				}
				break;
			}

			//walk in a direction until hitting a wall
			switch(this->defaultOrientation)
			{
			case 1:
				if (grid->getNorthNode(this->selfNode) != NULL)
				{
					selfNode = grid->getNorthNode(selfNode);
					mWalkList.push_back(grid->getPosition(selfNode->getRow(), selfNode->getColumn()));
				}
				break;
			case 2:
				if (grid->getSouthNode(this->selfNode) != NULL)
				{
					selfNode = grid->getSouthNode(selfNode);
					mWalkList.push_back(grid->getPosition(selfNode->getRow(), selfNode->getColumn()));
				}
				break;
			case 3:
				if (grid->getEastNode(this->selfNode) != NULL)
				{
					selfNode = grid->getEastNode(selfNode);
					mWalkList.push_back(grid->getPosition(selfNode->getRow(), selfNode->getColumn()));
				}
				break;
			case 4:
				if (grid->getWestNode(this->selfNode) != NULL)
				{
					selfNode = grid->getWestNode(selfNode);
					mWalkList.push_back(grid->getPosition(selfNode->getRow(), selfNode->getColumn()));
				}
				break;
			}
		}
		
	}

	//GHOSTS
	if(agentType == 'g'){ //if ghost

		//make a list of intersections if there isn't one already
		if(intersections.size() == 0){
			GridNode *temp;
			for(int i = 0; i <= grid->getColNum(); i++){
				for(int j = 0; j <= grid->getRowNum(); j++){
					temp = grid->getNode(i,j);
					if(temp){ //if not null
						if(temp->isClear()){
							//if there is a turn at this grid node
							if( (grid->getNorthNode(temp) && grid->getEastNode(temp) ) || (grid->getNorthNode(temp) && grid->getWestNode(temp) )  
								|| (grid->getSouthNode(temp) && grid->getEastNode(temp)) || (grid->getSouthNode(temp) && grid->getWestNode(temp)) )
							{
								intersections.push_back(temp);
							}
						}
					}
				}
			}
			//find special spots where teleporters are located
			for(int i = 0; i<19; i++){
				GridNode *temp = grid->getNode(i, 0);
				if(temp->isClear()){
					intersections.push_back(grid->getNode(i, 0));
				}
			}
			for(int i = 0; i<19; i++){
				GridNode *temp = grid->getNode(i, 18);
				if(temp->isClear()){
					intersections.push_back(grid->getNode(i, 18));
				}
			}
			for(int i = 0; i<19; i++){
				GridNode *temp = grid->getNode(0, i);
				if(temp->isClear()){
					intersections.push_back(grid->getNode(0, i));
				}
			}
			for(int i = 0; i<19; i++){
				GridNode *temp = grid->getNode(18, i);
				if(temp->isClear()){
					intersections.push_back(grid->getNode(18, i));
				}
			}
		}

		//start and goal for A*
		GridNode *current;
		GridNode *goal;

		//find out if ghost can see player
		bool LOS = true;
		if(selfNode->getColumn() == player->getSelfNode()->getColumn()){ //if ghost and player are in the same column
			int dist = selfNode->getRow() - player->getSelfNode()->getRow(); //find distance to player from ghost
			if(abs(dist) < 7){ //if player is in range
				if(dist < 0){ //if ghost is on the player's left
					GridNode *temp = selfNode;
					for(int i=0; i>=dist; i--){
						temp = grid->getEastNode(temp); //look right
						if(temp){
							if(!temp->isClear()){ //declare player out of line of sight if a wall is in the way
								LOS=false;
							}
						}
						else{ //declare player out of line of sight if a boundry is in the way
							LOS=false;
						}
					}
				}
				else if(dist > 0){ //if ghost is on the player's right
					GridNode *temp = selfNode;
					for(int i=0; i<=dist; i++){
						temp = grid->getEastNode(temp); //look right
						if(temp){
							if(!temp->isClear()){ //declare player out of line of sight if a wall is in the way
								LOS=false;
							}
						}
						else{ //declare player out of line of sight if a boundry is in the way
							LOS=false;
						}
					}
				}
			}
		}
		else if(selfNode->getRow() == player->getSelfNode()->getRow()){ //if ghost and player are in the same row
			int dist = selfNode->getColumn() - player->getSelfNode()->getColumn();
			if(abs(dist) < 7){
				if(dist < 0){ //if ghost is on the player's south
					GridNode *temp = selfNode;
					for(int i=0; i>dist; i--){
						temp = grid->getNorthNode(temp); //look north
						if(temp){
							if(!temp->isClear()){ //declare player out of line of sight if a wall is in the way
								LOS=false;
							}
						}
						else{ //declare player out of line of sight if a boundry is in the way
							LOS=false;
						}
					}
				}
				else if(dist > 0){ //if ghost is on the player's north
					GridNode *temp = selfNode;
					for(int i=0; i<dist; i++){
						temp = grid->getSouthNode(temp); //look south
						if(temp){
							if(!temp->isClear()){ //declare player out of line of sight if a wall is in the way
								LOS=false;
							}
						}
						else{ //declare player out of line of sight if a boundry is in the way
							LOS=false;
						}
					}
				}
			}
		}
		else{ //declare player out of line of sight if not in a line with player
			LOS=false;
		}

		//if ghost is not moving 
		if(mWalkList.empty()){
			current = selfNode;
			//if player is in LOS
			if(LOS){
				goalNode = player->getSelfNode(); //target player
			}
			//else if not in LOS
			else{
				setGoalNode(); //pick a random goal
			}
			goal = goalNode;
		}
		//else if ghost is moving
		else{
			return; //return if still in route to goal
		}

		//begin A* /////////////////////////////////////////////////////////////////////////
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
			//adjusted A* for no diagnols since this is like pacman
			temp.clear(); //clear previous neighbors
			GridNode *node = NULL;
			node = grid->getEastNode(current);
			if(node){
				if(node->isClear()){
					temp.push_front(node);
				}
			}
			node = grid->getNorthNode(current);
			if(node){
				if(node->isClear()){
					temp.push_front(node);
				}
			}
			node = grid->getWestNode(current);
			if(node){
				if(node->isClear()){
					temp.push_front(node);
				}
			}
			node = grid->getSouthNode(current);
			if(node){
				if(node->isClear()){
					temp.push_front(node);
				}
			}
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
						if( grid->getDistance((*i), bestN) == 1){
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

				//backtrack the path to the goal into the walklist
				while( current != selfNode ){
					mWalkList.push_front(grid->getPosition(current->getRow(), current->getColumn()));

					//traverse to parent
					current = current->parent;

				}
				//erase parents so that other A*s running don't get tangled
				grid->eraseParents();

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

void
Agent::collide(Ogre::Real deltaTime)
{
	if (agentType == 'g')
	{
		Ogre::AxisAlignedBox ghostBox = this->mBodyEntity->getWorldBoundingBox(true);		//get ghost's hitbox
		Ogre::AxisAlignedBox playerBox = player->mBodyEntity->getWorldBoundingBox(true);	//get player's hitbox
		if (ghostBox.intersects(playerBox.getCenter())){									//check for a collision between the ghost's hitbox the end center of the player
			if(player)
				player->loseLife();	//player loses a life when hit by a ghost 
		}
	}
}

void
Agent::loseLife()
{
	lives--;
	pauseTimer = Ogre::Real(5);	//pause game while repositioning occurs
	if(lives <= 0){
		reset = true; //reset game when you are out of lives
	}

}

//when the player dies, this function is called to reset the map to how it is at the start.
void
Agent::resetPositions(Ogre::Real deltaTime){
	pauseTimer -= deltaTime;										//count down time
	if(agentType == 'c' || agentType == 'g')
	{
		int y = startNode->getColumn(), x = startNode->getRow();	//select startNode(spawn) as teleporter exit
		mBodyNode->setPosition(grid->getPosition(x,y));				//move everything over
		mDestination = grid->getPosition(x,y);						//set destination to exit
		mWalkList.clear();											//clear any pathfinding
		mDistance = 0;												//clear distance since already here
		//mWalkList.push_front(grid->getPosition(x,y));				//walk here (just in case)
		selfNode = grid->getNode(x,y);								//tell agent where it is
		goalNode = grid->getNode(x,y);								//make agent want to be here
		setBaseAnimation(ANIM_IDLE_BASE);							//hold animation in place
		setTopAnimation(ANIM_IDLE_TOP);
		mDirection = Ogre::Vector3::ZERO;							//face default direction while held in place
		Ogre::Vector3 src = mBodyNode->getOrientation() * Ogre::Vector3::UNIT_Z;
		Ogre::Quaternion quat = src.getRotationTo(mDirection);
		mBodyNode->setOrientation(quat);
	}
}
