import sys
import heapq
import copy

class Resource:
    def __init__(self, RI, RA, RP, RW, RM, RL, RU, RT, RE=None):
        self.RI = RI  # Resource ID
        self.RA = RA  # Activation Cost
        self.RP = RP  # Periodic Maintenance Cost
        self.RW = RW  # Active Turns
        self.RM = RM  # Maintenance Downtime Turns
        self.RL = RL  # Total Life Cycle
        self.RU = RU  # Buildings Powered per Turn
        self.RT = RT  # Resource Type
        self.RE = RE  # Special Effect Value
        self.remaining_active_turns = RW
        self.remaining_life = RL
        self.downtime = 0  # Tracks maintenance downtime
    
    def apply_effect(self, game_state):
        if self.RT == 'A':  # Smart Meter
            for r in game_state.active_resources:
                r.RU = max(0, int(r.RU * (1 + self.RE / 100)))
        elif self.RT == 'B':  # Distribution Facility
            game_state.TM = max(0, int(game_state.TM * (1 + self.RE / 100)))
            game_state.TX = max(0, int(game_state.TX * (1 + self.RE / 100)))
        elif self.RT == 'C':  # Maintenance Plan
            for r in game_state.active_resources:
                r.RL = max(1, int(r.RL * (1 + self.RE / 100)))
        elif self.RT == 'D':  # Renewable Plant
            game_state.TR = max(0, int(game_state.TR * (1 + self.RE / 100)))
        elif self.RT == 'E':  # Accumulator
            game_state.accumulator += self.RE
    
    def is_active(self):
        return self.remaining_active_turns > 0 and self.downtime == 0
    
    def decrement_turns(self):
        if self.remaining_active_turns > 0:
            self.remaining_active_turns -= 1
        elif self.downtime > 0:
            self.downtime -= 1
        self.remaining_life -= 1

        if self.remaining_active_turns == 0 and self.downtime == 0 and self.remaining_life > 0:
            self.downtime = self.RM
        elif self.downtime == 0 and self.remaining_active_turns == 0:
            self.remaining_active_turns = self.RW

class GameState:
    def __init__(self, D, turn_index=0, active_resources=None, total_profit=0, accumulator=0, purchases=None):
        self.D = D  # Budget
        self.turn_index = turn_index  # Current turn index
        self.active_resources = active_resources if active_resources is not None else []
        self.total_profit = total_profit  # Total profit accumulated
        self.accumulator = accumulator  # Energy storage from E-type resources
        self.purchases = purchases if purchases is not None else []  # Track purchases per turn
    
    def update_turn_params(self, TM, TX, TR):
        self.TM, self.TX, self.TR = TM, TX, TR
    
    def apply_resource_effects(self):
        for resource in self.active_resources:
            resource.apply_effect(self)
    
    def compute_profit(self):
        total_power = sum(r.RU for r in self.active_resources if r.is_active())
        
        if total_power < self.TM and self.accumulator > 0:
            deficit = self.TM - total_power
            used = min(self.accumulator, deficit)
            total_power += used
            self.accumulator -= used

        if total_power < self.TM:
            return -sum(r.RP for r in self.active_resources if r.is_active())
        
        profit = min(total_power, self.TX) * self.TR
        maintenance_costs = sum(r.RP for r in self.active_resources if r.is_active())
        return profit - maintenance_costs
    
    def __lt__(self, other):
        return self.total_profit > other.total_profit  # Higher profit states have higher priority

def parse_input(filename):
    with open(filename, 'r') as f:
        lines = f.read().strip().split('\n')
    
    D, R, T = map(int, lines[0].split())
    resources = []
    
    for i in range(1, R + 1):
        parts = lines[i].split()
        RI, RA, RP, RW, RM, RL, RU, RT = int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]), int(parts[5]), int(parts[6]), parts[7]
        RE = int(parts[8]) if len(parts) > 8 else None
        resources.append(Resource(RI, RA, RP, RW, RM, RL, RU, RT, RE))
    
    turns = []
    for i in range(R + 1, R + 1 + T):
        TM, TX, TR = map(int, lines[i].split())
        turns.append({'TM': TM, 'TX': TX, 'TR': TR})
    
    return D, resources, turns

def simulate_game(D, resources, turns, output_file):
    initial_state = GameState(D)
    priority_queue = [initial_state]
    best_final_state = None
    
    while priority_queue:
        game_state = heapq.heappop(priority_queue)
        
        if game_state.turn_index >= len(turns):
            if best_final_state is None or game_state.total_profit > best_final_state.total_profit:
                best_final_state = game_state
            continue
        
        turn = turns[game_state.turn_index]
        game_state.update_turn_params(turn['TM'], turn['TX'], turn['TR'])
        
        new_state = copy.deepcopy(game_state)
        turn_purchases = []
        
        for res in resources:
            if new_state.D >= res.RA:
                new_state.D -= res.RA
                new_state.active_resources.append(res)
                turn_purchases.append(res.RI)
                new_state.apply_resource_effects()
                
        new_state.total_profit += new_state.compute_profit()
        new_state.turn_index += 1
        if turn_purchases:
            new_state.purchases.append((new_state.turn_index - 1, turn_purchases))
        heapq.heappush(priority_queue, new_state)
    
    with open(output_file, 'w') as f:
        for turn, resources in best_final_state.purchases:
            f.write(f"{turn} {len(resources)} " + " ".join(map(str, resources)) + "\n")
    
    return best_final_state.total_profit

if __name__ == "__main__":
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    D, resources, turns = parse_input(input_file)
    final_score = simulate_game(D, resources, turns, output_file)
    print("Final Score:", final_score)

