% serhan cakmak
% 2020400225
% compiling: yes
% complete: yes
distance(0, 0, 0).  % a dummy predicate to make the sim work.


distance(Agent, TargetAgent, Distance):- X_dist is abs(Agent.x - TargetAgent.x), Y_dist is abs(Agent.y - TargetAgent.y), Distance is X_dist + Y_dist. 



%I added new predicates that indicate the travel cost for classes.
travel_cost(warrior, 5).
travel_cost(wizard, 2).
travel_cost(rogue, 5).
multiverse_distance(StateId, AgentId, TargetStateId, TargetAgentId, Distance):-                 
    state( StateId, Firstagents, _, _ ), state( TargetStateId, SecondAgents, _, _ ), history(StateId,U1, T1, _ ), history(TargetStateId, U2, T2, _ ),
     get_dict(AgentId, Firstagents, Agent1),  get_dict(TargetAgentId, SecondAgents, Agent2), distance(Agent1, Agent2,DistanceXY),
     travel_cost(Agent1.class, Cost), Distance is (DistanceXY + Cost * (abs(T1- T2)+ abs(U1 - U2))) .



%Gather all the agents in the same state and calculate the distance between them and the agent with the given id. Sort them to find the nearest one.
nearest_agent(StateId, AgentId, NearestAgentId, Distance) :-
    state(StateId, Agents, _, _), get_dict(AgentId, Agents, Agent1),
    findall( [Distance1, AgentId1],
            (get_dict(AgentId1, Agents, FilteredAgent),
            Agent1.name \= FilteredAgent.name,
            distance( Agent1 ,FilteredAgent, Distance1)),
            Distances
          
            ),
    sort(Distances, SortedDistances),
    SortedDistances = [[Distance, NearestAgentId]|_],
    !
    .




%same logic as nearest_agent but for multiverse.
nearest_agent_in_multiverse(StateId, AgentId, TargetStateId, TargetAgentId, Distance):-

    state(StateId, Agents, _, _), get_dict(AgentId, Agents, Agent1),
    findall( [Distance1,TargetStateId1, AgentId1 ],
            (
            state(TargetStateId1, AllAgents, _, _),         
            get_dict(AgentId1, AllAgents, FilteredAgent),
            Agent1.name \= FilteredAgent.name,
            multiverse_distance(StateId, AgentId, TargetStateId1, AgentId1, Distance1)
            ),
            Distances
            
            ),
    sort(Distances, SortedDistances), 
    SortedDistances = [[Distance, TargetStateId,TargetAgentId]|_],
    !.

% count the number of agents in the same state using length function.
num_agents_in_state(StateId, Name, NumWarriors, NumWizards, NumRogues):- 
    state(StateId, AllAgents, _, _), 
    findall(
        Class,
        
        (
        	get_dict(_,AllAgents, Agents),
        	get_dict(class, Agents, Class),
        	Class == warrior,
        	get_dict(name, Agents, Name1),
        	Name1 \= Name
        
        ),
    	WarriorList),
    	length(WarriorList, NumWarriors),
    
        findall(
        Class,
        
        (
        	get_dict(_,AllAgents, Agents),
        	get_dict(class, Agents, Class),
        	Class == wizard,
        	get_dict(name, Agents, Name1),
        	Name1 \= Name
        
        ),
    	WizardList),
    	length(WizardList, NumWizards),
    
     findall(
        Class,
        
        (
        	get_dict(_,AllAgents, Agents),
        	get_dict(class, Agents, Class),
        	Class == rogue,
        	get_dict(name, Agents, Name1),
        	Name1 \= Name
        
        ),
    	RogueList),
    	length(RogueList, NumRogues).

% 3 functions with the same signature to calculate the difficulty of the state.
difficulty_of_state(StateId, Name, AgentClass, Difficulty):-
    AgentClass = warrior,
    num_agents_in_state(StateId, Name, NumWarriors, NumWizards, NumRogues),
    Difficulty is 5 * NumWarriors + 8 * NumWizards + 2 * NumRogues, !.



difficulty_of_state(StateId, Name, AgentClass, Difficulty):-
    AgentClass = wizard,
    num_agents_in_state(StateId, Name, NumWarriors, NumWizards, NumRogues),
    Difficulty is 2 * NumWarriors + 5 * NumWizards + 8 * NumRogues, !.

difficulty_of_state(StateId, Name, AgentClass, Difficulty):-
    AgentClass = rogue,
    num_agents_in_state(StateId, Name, NumWarriors, NumWizards, NumRogues),
    Difficulty is 8 * NumWarriors + 2 * NumWizards + 5 * NumRogues, !.
    



% Condition checks for portal and portal_to_now.
isTraversable(StateId, AgentId, TargetStateId, Action):-
    state(StateId, Agents, _, TurnOrder),
    history(StateId, UniverseId, Time, _), 
    history(TargetStateId, TargetUniverseId, TargetTime, _),
    get_dict(AgentId, Agents, Agent) ,(
    
    (global_universe_id(GlobalUniverseId),
    universe_limit(UniverseLimit),
    GlobalUniverseId < UniverseLimit,
    % agent cannot time travel if there is only one agent in the universe
    length(TurnOrder, NumAgents),
    NumAgents > 1,
    
    % check whether target is now or in the past
    current_time(TargetUniverseId, TargetUniCurrentTime, _),
    TargetTime < TargetUniCurrentTime,
    % check whether there is enough mana
    (Agent.class = wizard -> TravelCost = 2; TravelCost = 5),
    Cost is abs(TargetTime - Time)*TravelCost + abs(TargetUniverseId - UniverseId)*TravelCost,
    Agent.mana >= Cost,
    % check whether the target location is occupied
    get_earliest_target_state(TargetUniverseId, TargetTime, TargetStateId),
    state(TargetStateId, TargetAgents, _, TargetTurnOrder),
    TargetState = state(TargetStateId, TargetAgents, _, TargetTurnOrder),
    \+tile_occupied(Agent.x, Agent.y, TargetState),
    Action = portal
    );

    (
    length(TurnOrder, NumAgents),
    NumAgents > 1,
    % [TargetUniverseId] = ActionArgs,
    % agent can only travel to now if it's the first turn in the target universe
    current_time(TargetUniverseId, TargetTime, 0),
    % agent cannot travel to current universe's now (would be a no-op)
    \+(TargetUniverseId = UniverseId),
    % check whether there is enough mana
    (Agent.class = wizard -> TravelCost = 2; TravelCost = 5),
    Cost is abs(TargetTime - Time)*TravelCost + abs(TargetUniverseId - UniverseId)*TravelCost,
    Agent.mana >= Cost,
    % check whether the target location is occupied
    get_latest_target_state(TargetUniverseId, TargetTime, TargetStateId),
    state(TargetStateId, TargetAgents, _, TargetTurnOrder),
    TargetState = state(TargetStateId, TargetAgents, _, TargetTurnOrder),
    \+tile_occupied(Agent.x, Agent.y, TargetState),
    Action = portal_to_now
    )).

% Get the least difficult state that is traversable. Including the current state.
easiest_traversable_state(StateId, AgentId, TargetStateId):-
    
    state(StateId, Agents, _ , _),
    get_dict(AgentId, Agents,Agent),
    findall( [ Difi,TargetState1 ],
    (
    isTraversable(StateId, AgentId, TargetState1, _),
    difficulty_of_state(TargetState1,Agent.name, Agent.class, Difi ),
    Difi > 0
    ),
    TargetStateIdList),                                     
    difficulty_of_state(StateId, Agent.name, Agent.class, Difi1),
    sort(TargetStateIdList, SortedTargetStateIdList), 
    SortedTargetStateIdList = [[Difi2,TargetStateId1]|_],
    ( (Difi1 < Difi2), (Difi1 > 0) -> TargetStateId = StateId; TargetStateId = TargetStateId1 )
    .

% Almost same as easiest_traversable_state, but this one also returns actions and difficulties for 8th function.
modi(StateId, AgentId, TargetStateId, Difficulty, Action):-
    % isTraversable(StateId, AgentId, TargetStateId).
    state(StateId, Agents, _ , _),
    get_dict(AgentId, Agents,Agent),
    findall( [ Difi,TargetState1 ,Action1],
    (
    isTraversable(StateId, AgentId, TargetState1, Action1),
    difficulty_of_state(TargetState1,Agent.name, Agent.class, Difi ),
    Difi > 0
    ),
    TargetStateIdList),
    sort(TargetStateIdList, SortedTargetStateIdList),
    SortedTargetStateIdList = [[Difficulty,TargetStateId, Action]|_].


% go 0 = left, 1 = right, 2 = down, 3 = up
getDirection(Agent1, Agent2, OpNum):-
    Agent1.x = X1,
    Agent2.x = X2,
    X1 > X2,                    %go left
    OpNum = 0, !.
getDirection(Agent1, Agent2, OpNum):-
    Agent1.x = X1,
    Agent2.x = X2,
    X1 < X2,                    %go right
    OpNum = 1, !.

getDirection(Agent1, Agent2, OpNum):-
    Agent1.y = Y1,
    Agent2.y = Y2,
    Y1 > Y2,                    %go down
    OpNum = 2, !.

getDirection(Agent1, Agent2, OpNum):-
    Agent1.y = Y1,
    Agent2.y = Y2,
    Y1 < Y2,                    %go up
    OpNum = 3, !.


basic_action_policy(StateId, AgentId, Action):-
    state(StateId, Agents, _, _),
    get_dict(AgentId, Agents, Agent),
    modi(StateId,AgentId, TargetStateId, D2 ,Action1),                      
    difficulty_of_state(StateId,Agent.name, Agent.class, D1), 
    (D1 is 0 ;D1 > D2),
    history(TargetStateId,TargetUniverseId, _, _ ),                                 
    Action = [Action1,TargetUniverseId ], !.

basic_action_policy(StateId, AgentId, Action):-
    state(StateId, Agents, _, _), 
    get_dict(AgentId, Agents, Agent),nearest_agent(StateId, AgentId, TargetAgentId , Distance),
    ((Agent.class = warrior -> AttackRange = 1  , Action  = [melee_attack, TargetAgentId] ) ; (Agent.class = wizard -> AttackRange = 10,
    Action  = [magic_missile, TargetAgentId]  );
    (Agent.class = rogue -> AttackRange = 5 , Action  = [ranged_attack, TargetAgentId])),
    
    Distance =< AttackRange, !.


basic_action_policy(StateId, AgentId, Action):-
    state(StateId, Agents, _, _),
    nearest_agent(StateId, AgentId, NearestAgentId, _),
    get_dict(AgentId, Agents, Agent),
    get_dict(NearestAgentId, Agents, TargetAgent),
    getDirection(Agent, TargetAgent, OpNum), 
    (OpNum is 0 -> Action = [move_left]; OpNum is 1 -> Action = [move_right];
    OpNum is 2 -> Action = [move_down]; OpNum is 3 -> Action = [move_up]), !.

basic_action_policy(_, _, Action):-
    Action = [rest], !.
        
       
       

