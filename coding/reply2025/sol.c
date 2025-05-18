#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <stdlib.h>
#include <math.h>
#include <unordered_map>
#include <random>
#include <numeric>
#include <array>
#include <assert.h>

//#define DEBUG

int test;

const std::string tests[] = {
    "0-demo",
    "1-thunberg",
    "2-attenborough",
    "3-goodall",
    "4-maathai",
    "5-carson",
    "6-earle",
    "7-mckibben",
    "8-shiva"
};

int testNum;

std::mt19937 generator;
int randNum(int lb, int ub)
{
    std::uniform_int_distribution<int> distribution(lb, ub - 1);
    return distribution(generator);
}

const long long INF = 1e12;
const long long MAX_RES_PER_TURN = 50;

long long initCap;
long long numRes;
long long numTurns;

#define ST_NONE 0
#define ST_NUM_BUILDS 1
#define ST_MINMAX_BUILDS 2
#define ST_LIFE_EXT 3
#define ST_PROFIT 4
#define ST_ACCUM 5

struct Res
{
    int id;
    long long initCost;
    long long turnCost;
    long long actTurns;
    long long downTurns;
    long long lifeTurns;
    long long numBuilds;
    int specType;
    long long specEff = 0;
};

struct Turn
{
    long long minBuilds;
    long long maxBuilds;
    long long profPerBuild;
};

std::vector<Res> res;
std::vector<Turn> turns;

void input()
{
    std::ifstream inFile;
    inFile.open((tests[testNum] + ".txt").c_str());

    inFile >> initCap;
    inFile >> numRes;
    inFile >> numTurns;

    for (int i = 0; i < numRes; i++)
    {
        res.push_back({});
        Res& r = res.back();

        inFile >> r.id;
        inFile >> r.initCost;
        inFile >> r.turnCost;
        inFile >> r.actTurns;
        inFile >> r.downTurns;
        inFile >> r.lifeTurns;
        inFile >> r.numBuilds;

        char st;
        inFile >> st;

        if (st == 'X') r.specType = ST_NONE;
        if (st == 'A') r.specType = ST_NUM_BUILDS;
        if (st == 'B') r.specType = ST_MINMAX_BUILDS;
        if (st == 'C') r.specType = ST_LIFE_EXT;
        if (st == 'D') r.specType = ST_PROFIT;
        if (st == 'E') r.specType = ST_ACCUM;

        if (r.specType != ST_NONE)
        {
            inFile >> r.specEff;
        }
    }

    for (int t = 0; t < numTurns; t++)
    {
        turns.push_back({});
        Turn& turn = turns.back();

        inFile >> turn.minBuilds;
        inFile >> turn.maxBuilds;
        inFile >> turn.profPerBuild;
    }
}

struct Sol
{
    long long score = 0;
    std::vector<std::vector<int>> rActs;

    Sol(long long score = 0):
        score(score),
        rActs(numTurns)
    {}
};

void output(const Sol& sol)
{
    std::ofstream outFile;
    outFile.open(("sol/sol_" + tests[testNum] + ".txt").c_str());

    for (int t = 0; t < numTurns; t++)
    {
        if (t >= (int) sol.rActs.size()) continue;
        if (sol.rActs[t].empty()) continue;
        outFile << t << " " << sol.rActs[t].size();
        for (int i : sol.rActs[t])
        {
            outFile << " " << i;
        }
        outFile << "\n";
    }
}

Sol bestSol(-INF);

void updateSol(const Sol& sol)
{
    if (sol.score <= bestSol.score) return;

    std::cerr << "New best score: " << sol.score / 1000000000ll << " " << sol.score << std::endl;

    bestSol = sol;
}

struct ActRes
{
    int idx = 0;

    long long startTime = 0;
    long long lifetime = 0;
};

long long applyPerc(long long base, long long perc, long long min=0)
{
    return std::max(base + (base * perc) / 100, min);
}

void solve()
{
    Sol sol;

    long long cap = initCap;
    // long long accum = 0;

    std::vector<ActRes> actRes;

    for (int t = 0; t < numTurns; t++)
    {
        long long minRawLifeTime = numTurns;

        for (int i = 0; i < (int) actRes.size(); i++)
        {
            if (t - actRes[i].startTime >= actRes[i].lifetime)
            {
                std::swap(actRes[i], actRes.back());
                actRes.pop_back();
                i--;
            }
            else if (actRes[i].startTime >= t - 30)
            {
                minRawLifeTime = std::min(minRawLifeTime, res[actRes[i].idx].lifeTurns);
            }
        }

        std::vector<long long> nbv;
        long long numBuilds = 0;
        long long minBuilds = turns[t].minBuilds;
        long long maxBuilds = turns[t].maxBuilds;
        long long profPerBuild = turns[t].profPerBuild;

        long long numBuildsPerc = 0;
        long long minMaxBuildsPerc = 0;
        long long lifetimePerc = 0;
        long long ppbPerc = 0;
        // long long maxAccum = 0;

        long long finalNumBuilds = numBuilds;
        long long finalMinBuilds = minBuilds;
        long long finalMaxBuilds = maxBuilds;
        long long finalProfPerBuild = profPerBuild;

        auto applyResource = [&](const Res& r, bool inv = false)
        {
            int sign = 1;
            if (inv) sign = -1;

            numBuilds += r.numBuilds * sign;

            if (!inv) nbv.push_back(r.numBuilds);
            else nbv.pop_back();

            if (r.specType == ST_NUM_BUILDS) numBuildsPerc += r.specEff * sign;
            if (r.specType == ST_MINMAX_BUILDS) minMaxBuildsPerc += r.specEff * sign;
            if (r.specType == ST_LIFE_EXT) lifetimePerc += r.specEff * sign;
            if (r.specType == ST_PROFIT) ppbPerc += r.specEff * sign;
            // if (r.specType == ST_ACCUM) maxAccum += r.specEff * sign;

            finalNumBuilds = applyPerc(numBuilds, numBuildsPerc);
            finalMinBuilds = applyPerc(minBuilds, minMaxBuildsPerc);
            finalMaxBuilds = applyPerc(maxBuilds, minMaxBuildsPerc);
            finalProfPerBuild = applyPerc(profPerBuild, ppbPerc);
        };

        auto computeProfAppr = [&]()
        {
            long long b = std::min(finalNumBuilds, finalMaxBuilds);
            long long prof = b * finalProfPerBuild;

            if (b < minBuilds) prof = prof * b / minBuilds;

            return applyPerc(prof, std::min(lifetimePerc, 100ll * (numTurns - t) / minRawLifeTime));
        };

        for (const ActRes& ar : actRes)
        {
            const Res& r = res[ar.idx];

            int phase = (t - ar.startTime) % (r.actTurns + r.downTurns);

            if (phase >= r.actTurns) continue;

            applyResource(r);
        }

        for (int numBought = 0; numBought < MAX_RES_PER_TURN; numBought++)
        {
            double maxScore = -INF;
            int bestI = -1;

            bool needLifetime = applyPerc(minRawLifeTime, lifetimePerc) < numTurns - t;

            for (int i = 0; i < numRes; i++)
            {
                const Res& r = res[i];

                // std::cerr << "  t=" << t << " j=" << numBought << " i=" << i << " :: " << r.initCost << " / " << cap << std::endl;

                if (r.initCost > cap) continue;

                // if (r.specType == ST_NONE) continue;
                // if (r.specType == ST_NUM_BUILDS) continue;
                // if (r.specType == ST_LIFE_EXT) continue;
                // if (r.specType == ST_MINMAX_BUILDS) continue;
                // if (r.specType == ST_PROFIT) continue;
                // if (r.specType == ST_ACCUM) continue;

                if (r.specType == ST_LIFE_EXT && !needLifetime) continue;

                long long currProf = computeProfAppr();

                applyResource(r);

                long long newProf = computeProfAppr();
                long long profDelta = newProf - currProf;

                long long lifeTime = applyPerc(r.lifeTurns, lifetimePerc, 1);
                lifeTime = std::min(lifeTime, numTurns - t);

                applyResource(r, true);

                long long cycleTime = r.actTurns + r.downTurns;
                long long actTime = lifeTime / cycleTime * r.actTurns + std::min(lifeTime % cycleTime, r.actTurns); 

                long long totalProf = profDelta * actTime;
                long long totalCap = totalProf - r.turnCost * lifeTime - r.initCost;

                // long long score = totalProf * std::min(MAX_RES_PER_TURN - numBought, cap / r.initCost);

                long long score = totalProf * std::min(MAX_RES_PER_TURN - numBought, cap / r.initCost);

                if (t >= 10 && r.specType == ST_LIFE_EXT && needLifetime) score *= 6;

                // std::cerr << "    delta=" << profDelta << " lifeTime=" << lifeTime << " actTime=" << actTime << " score=" << score << std::endl;

                if (totalCap > 0 && score > maxScore)
                {
                    maxScore = score;
                    bestI = i;
                }
            }

            if (bestI == -1) break;

            const Res& r = res[bestI];

            // std::cerr << "    t=" << t << " j=" << numBought << " id=" << r.id << " :: " << r.initCost << " / " << cap << std::endl;

            sol.rActs[t].push_back(r.id);

            cap -= r.initCost;

            actRes.push_back({bestI, t, r.lifeTurns});

            applyResource(r);
        }

        finalNumBuilds = 0;
        for (long long nb : nbv)
        {
            finalNumBuilds += applyPerc(nb, numBuildsPerc);
        }

        // std::cerr << "  t=" << t << " numBuildsRaw=" << numBuilds << " nbp=" << numBuildsPerc << " -> " << finalNumBuilds << std::endl;

        numBuilds = finalNumBuilds;
        minBuilds = finalMinBuilds;
        maxBuilds = finalMaxBuilds;
        profPerBuild = finalProfPerBuild;

        for (const ActRes& ar : actRes)
        {
            const Res& r = res[ar.idx];
            cap -= r.turnCost;
        }

        // std::cerr << "  t=" << t << " paid ::" << " cap=" << cap << std::endl;

        for (ActRes& ar : actRes)
        {
            if (ar.startTime < t) continue;

            ar.lifetime = applyPerc(ar.lifetime, lifetimePerc, 1);
        }

        // accum = std::min(accum, maxAccum);

        // if (numBuilds < minBuilds && numBuilds + accum >= maxBuilds)
        // {
        //     accum -= minBuilds - numBuilds;
        //     numBuilds = minBuilds;
        // }

        // if (numBuilds > maxBuilds)
        // {
        //     // accum += numBuilds - maxBuilds;
        // }

        // accum = std::min(accum, maxAccum);

        long long profit = numBuilds >= minBuilds ? std::min(numBuilds, maxBuilds) * profPerBuild : 0;

        sol.score += profit;

        cap += profit;
 
        // std::cerr << "  t=" << t << " ::" << " numBuilds=" << numBuilds << " / " << minBuilds << "-" << maxBuilds << " ppb=" << profPerBuild << " profit=" << profit << " cap=" << cap << std::endl;
        std::cerr << "  t=" << t << " ::" << " nbp=" << numBuildsPerc << " mmbp=" << minMaxBuildsPerc << " ppbp=" << ppbPerc << " lp=" << lifetimePerc << std::endl;
    }

    updateSol(sol);
}

int main()
{
    std::cin >> testNum;

    generator.seed(0);

    input();

    std::cerr << "Input done" << std::endl;

    solve();

    std::cerr << "Solve done" << std::endl;

    output(bestSol);

    std::cerr << "Output done" << std::endl;

    return 0;
}
