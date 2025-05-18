#include <bits/stdc++.h>
using namespace std;
 
#define FOR(i,a,b) for (int i = (a); i < (b); i++)
#define RFOR(i,b,a) for (int i = (b) - 1; i >= (a); i--)
#define ITER(it,a) for (__typeof(a.begin()) it = a.begin(); it != a.end(); it++)
#define FILL(a,value) memset(a, value, sizeof(a))
 
#define SZ(a) (int)a.size()
#define ALL(a) a.begin(), a.end()
#define PB push_back
#define MP make_pair
 
typedef long long Int;
typedef long long UInt;
typedef vector<int> VI;
typedef pair<int, int> PII;
 
const double PI = acos(-1.0);
const int INF = 1000 * 1000 * 1000;
const Int LINF = INF * (Int) INF;
const int MAX = 200007;
const int MOD = 998244353;

struct Resourse {
    int ri;
    int ra;
    int rp;
    int rw;
    int rm;
    int rl;
    int ru;
    char rt;
    int re;
};

struct Turn{
    int mn;
    int mx;
    int p;
};

struct State {
    int money;
    int r;
    int t;

    vector<Resourse> resourses;
    vector<Turn> turns;

    friend istream& operator>>(istream& in, State& state) {
        in >> state.money >> state.r >> state.t;
        vector<Resourse> resourses;
        FOR(i, 0, state.r) {
            Resourse rr;
            in >> rr.ri >> rr.ra >> rr.rp >> rr.rw >> rr.rm >> rr.rl >> rr.ru >> rr.rt;
            if (rr.ri == i + 1 && i == 0)
                resourses.push_back(Resourse());
            if (rr.rt != 'X') {
                in >> rr.re;
            }
            resourses.push_back(rr);
        }
        vector<Turn> turns(state.t);
        FOR(i, 0, state.t) {
            in >> turns[i].mn >> turns[i].mx >> turns[i].p;
        }
        state.resourses = resourses;
        state.turns = turns;
        return in;
    }
};

struct Answer {
    vector<VI> res;

    friend istream& operator>>(istream& in, Answer& answer) {
        int t;
        while (in >> t) {
            while (SZ(answer.res) < t) {
                answer.res.push_back(VI());
            }
            int cnt;
            in >> cnt;
            VI A;
            FOR(i,0,cnt) {
                int r;
                in >> r;
                A.push_back(r);
            }
            answer.res.push_back(A);
        }
        return in;
    }

    friend ostream& operator<<(ostream& out, Answer& answer) {
        FOR(i,0,SZ(answer.res)) {
            if (SZ(answer.res[i]) == 0) {
                continue;
            }
            out << i << ' ' << SZ(answer.res[i]) << ' ';
            for(int r: answer.res[i]) {
                out << r << ' ';
            }
            out << endl;
        }
        return out;
    }
};


Int Apply(Int value, Int modifier) {
    return max(0LL, Int(value + 1.0 * value * modifier / 100));
}

Int score(State st, Answer ans) {
    Int initial_money = st.money;
    Int money = st.money;

    while (SZ(ans.res) < st.t) {
        ans.res.push_back(VI());
    }
    vector<Int> periodic_cost(st.t);
    vector<VI> active_resourses(st.t);

    Int acc = 0;
    Int final_score = 0;
    FOR(t,0,st.t) {
        Int cost = 0;
        for(int r: ans.res[t]) {
            // cerr << "R " << ' ' << r << ' ' << st.resourses[r].ra << endl;
            cost += st.resourses[r].ra;
        }
        // cerr << "COST " << t << ' ' << cost << ' ' << money << ' ' << final_score << endl;
        if (cost > money) {
            cerr << "Cost is too high, turn " << t << endl;
            ans.res[t].clear();
        }
        for(int r: ans.res[t]) {
            active_resourses[t].push_back(r);
        }
        Int a_mod = 0;
        Int b_mod = 0;
        Int c_mod = 0;
        Int d_mod = 0;
        for (int r: active_resourses[t]) {
            switch (st.resourses[r].rt) {
                case 'A':
                    a_mod += st.resourses[r].re;
                    break;
                case 'B':
                    b_mod += st.resourses[r].re;
                    break;
                case 'C':
                    c_mod += st.resourses[r].re;
                    break;
                case 'D':
                    d_mod += st.resourses[r].re;
                    break;
            } 
        }
        for(int r: ans.res[t]) {
            auto resource = st.resourses[r];
            money -= resource.ra;
            Int life = Apply(Int(resource.rl), c_mod);
            FOR(j,0,life) {
                if (t + j >= st.t)
                    break;
                periodic_cost[t + j] += resource.rp;
                if (j > 0 and j % (resource.rw + resource.rm) < resource.rw) {
                    active_resourses[t + j].push_back(r);
                }
            }
        }
        money -= periodic_cost[t];
        Int mn = Apply(Int(st.turns[t].mn), b_mod);
        Int mx = Apply(Int(st.turns[t].mx), b_mod);
        Int p = Apply(st.turns[t].p, d_mod);
        Int buildings = 0;
        for (int r: active_resourses[t]) {
            auto resource = st.resourses[r];
            buildings += resource.ru;
        }
        buildings = Apply(buildings, a_mod);
        Int capacity = 0;
        for (int r: active_resourses[t]) {
            auto resource = st.resourses[r];
            if (st.resourses[r].rt == 'E') {
                capacity += resource.re;
            }
        }
        acc = min(acc, capacity);
        if (buildings < mn && buildings + acc >= mn) {
            acc -= mn - buildings;
            buildings = mn;
        }
        if (buildings > mx) {
            acc = min(acc + buildings - mx, capacity);
            buildings = mx;
        }
        if (buildings >= mn) {
            money += buildings * p;
            final_score += buildings * p;
        }
        // cerr << "TURN " << t << ' ' << buildings << ' ' << mn << ' ' << mx << ' ' << p << endl;
        // if (t < 10)
        //     cerr << "MONEY " << money << endl;
    }
    return final_score;
}


int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(0);cin.tie(0);
    ifstream in(argv[1]);
    ifstream ans(argv[2]);

    State state;
    Answer answer;
    in >> state;
    // cerr << SZ(state.resourses) << endl;
    // for(auto resource: state.resourses) {
    //     if (resource.ru > 10) {
    //         cerr << resource.ri << ' ' << resource.ra << ' ' << resource.rp << ' ' << resource.rw << ' ' << resource.rm << ' ' << resource.rl << ' ' << resource.ru << ' ' << resource.rt << ' ' << resource.re << endl;
    //     }
    // }
    
    answer.res.resize(state.t);
    
    FOR(i,0,2) {
        answer.res[0].push_back(21);
    }
    answer.res[1].push_back(21);

    vector<int> V;
    FOR(j,0,2) { V.PB(14);}
    FOR(j,0,45) { V.PB(10);}
    FOR(j,0,9) { V.PB(12);}
    FOR(j,0,44) { V.PB(20);}


    FOR(i,2,10000) {
        for(int j = i % 2; j < SZ(V); j += 2) {
            answer.res[i].push_back(V[j]);
        }
    }
    // ans >> answer;

    Int s = score(state, answer);
    cerr << "FINAL SCORE " << (double)s << endl;

    cout << answer;
    return 0;
}
