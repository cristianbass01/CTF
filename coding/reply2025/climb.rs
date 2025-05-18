#![allow(non_snake_case)]

use itertools::Itertools;
use proconio::input;
use rand::prelude::*;

use crossbeam::thread as cb_thread;
use rand::Rng;
use rand_pcg::Pcg64;
use std::sync::{Arc, Mutex};

fn main() {
    get_time();
    let input = read_input();
    let out = Arc::new(Mutex::new(read_output(&input)));
    let best_score = Arc::new(Mutex::new(compute_score(&input, &out.lock().unwrap())));
    eprintln!("{:.3}: 0, {}", get_time(), *best_score.lock().unwrap());

    let num_threads = 1; // スレッド数を適宜設定
    cb_thread::scope(|s| {
        for _ in 0..num_threads {
            let input = &input;
            let out = Arc::clone(&out);
            let best_score = Arc::clone(&best_score);
            s.spawn(move |_| {
                let mut rng = Pcg64::seed_from_u64(8409328 + thread_id::get() as u64);
                let mut iter = 0;
                while get_time() < 60.0 * 4.0 {
                    let t = rng.gen_range(0..input.T);
                    let coin = rng.gen_range(0..3);
                    let mut out2;
                    {
                        let out_guard = out.lock().unwrap();
                        out2 = out_guard.clone();
                    }
                    let len = out2[t].len();

                    if coin == 0 {
                        // insert
                        if out2[t].len() == 50 {
                            continue;
                        }
                        out2[t].insert(rng.gen_range(0..=len), rng.gen_range(0..input.R));
                    } else if coin == 1 {
                        // remove
                        if out2[t].is_empty() {
                            continue;
                        }
                        out2[t].remove(rng.gen_range(0..len));
                    } else {
                        // change
                        if out2[t].is_empty() {
                            continue;
                        }
                        out2[t][rng.gen_range(0..len)] = rng.gen_range(0..input.R);
                    }

                    let next = compute_score(&input, &out2);
                    let mut best_score_guard = best_score.lock().unwrap();
                    if *best_score_guard < next {
                        eprintln!("{:.3}: {}, {}", get_time(), iter, next);
                        *best_score_guard = next;
                        *out.lock().unwrap() = out2;
                    }
                    iter += 1;
                }
            });
        }
    })
    .unwrap();

    write_output(&input, &out.lock().unwrap());
    eprintln!("!log score {}", compute_score(&input, &out.lock().unwrap()));
    eprintln!("Time = {:.3}", get_time());
}
// 入出力と得点計算

fn read_output(input: &Input) -> Vec<Vec<usize>> {
    let file = std::fs::read_to_string("/home/wata/Dropbox/Reply2025/chokudai/best.txt").unwrap();
    let mut out = vec![vec![]; input.T];
    for line in file.lines() {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        let mut ss = line.split_whitespace();
        let t = ss.next().unwrap().parse::<usize>().unwrap();
        let n = ss.next().unwrap().parse::<usize>().unwrap();
        for _ in 0..n {
            let i = ss.next().unwrap().parse::<usize>().unwrap();
            out[t].push(i);
        }
    }
    out
}

use std::io::prelude::*;
use tempfile::NamedTempFile;

fn compute_score(input: &Input, out: &Vec<Vec<usize>>) -> i64 {
    let mut temp_file = NamedTempFile::new().expect("failed to create tempfile");
    for t in 0..out.len() {
        if out[t].len() == 0 {
            continue;
        }
        writeln!(
            temp_file,
            "{} {} {}",
            t,
            out[t].len(),
            out[t].iter().map(|&i| input.resources[i].RI).join(" ")
        )
        .unwrap();
    }
    let temp_path = temp_file.path();
    let c = std::process::Command::new("/home/wata/git/reply2025/wata/calcscore")
        .arg("/home/wata/git/reply2025/wata/in.txt")
        .arg(&temp_path)
        .output()
        .unwrap();
    let output = String::from_utf8(c.stdout).unwrap();
    output.trim().parse().unwrap()
}

fn write_output(input: &Input, out: &Vec<Vec<usize>>) {
    for t in 0..out.len() {
        if out[t].len() == 0 {
            continue;
        }
        println!(
            "{} {} {}",
            t,
            out[t].len(),
            out[t].iter().map(|&i| input.resources[i].RI).join(" ")
        );
    }
}

#[allow(unused)]
#[derive(Clone, Debug)]
struct Input {
    D: i64,
    R: usize,
    T: usize,
    resources: Vec<Resource>,
    turns: Vec<Turn>,
}

#[allow(unused)]
#[derive(Clone, Debug)]
struct Resource {
    /// Resource identifier
    RI: usize,
    /// Activation cost
    RA: i64,
    /// Periodic cost for each turn of life
    RP: i64,
    /// Number of consecutive turns in which the resource is active and generates profit
    RW: usize,
    /// Number of downtime turns required after a full cycle of activity
    RM: usize,
    /// Total life cycle of the resource (in turns), including both active and downtime periods, after which the resource becomes obsolete
    RL: usize,
    /// Number of buildings the resource can power in each active turn
    RU: i64,
    /// Special effect of the resource
    RT: EffectType,
}

#[allow(unused)]
#[derive(Clone, Debug, Copy)]
enum EffectType {
    SmartMeter(i64),
    DistributionFacility(i64),
    MaintenancePlan(i64),
    RenewablePlant(i64),
    Accumulator(i64),
    BaseResource,
}

#[allow(unused)]
#[derive(Clone, Debug)]
struct Turn {
    min: i64,
    max: i64,
    profit: i64,
}

fn read_input() -> Input {
    input! {
        D: i64, R: usize, T: usize,
    }
    let mut resources = vec![];
    for _ in 0..R {
        input! {
            RI: usize, RA: i64, RP: i64, RW: usize, RM: usize, RL: usize, RU: i64, RT: char,
        }
        let RT = if RT != 'X' {
            input! {
                RE: i64,
            }
            match RT {
                'A' => EffectType::SmartMeter(RE),
                'B' => EffectType::DistributionFacility(RE),
                'C' => EffectType::MaintenancePlan(RE),
                'D' => EffectType::RenewablePlant(RE),
                'E' => EffectType::Accumulator(RE),
                _ => panic!(),
            }
        } else {
            EffectType::BaseResource
        };
        resources.push(Resource {
            RI,
            RA,
            RP,
            RW,
            RM,
            RL,
            RU,
            RT,
        });
    }
    let mut turns = vec![];
    for _ in 0..T {
        input! {
            tm: i64, tx: i64, tr: i64,
        }
        turns.push(Turn {
            min: tm,
            max: tx,
            profit: tr,
        });
    }
    Input {
        D,
        R,
        T,
        resources,
        turns,
    }
}

// ここからライブラリ

pub trait SetMinMax {
    fn setmin(&mut self, v: Self) -> bool;
    fn setmax(&mut self, v: Self) -> bool;
}
impl<T> SetMinMax for T
where
    T: PartialOrd,
{
    fn setmin(&mut self, v: T) -> bool {
        *self > v && {
            *self = v;
            true
        }
    }
    fn setmax(&mut self, v: T) -> bool {
        *self < v && {
            *self = v;
            true
        }
    }
}

#[macro_export]
macro_rules! mat {
	($($e:expr),*) => { vec![$($e),*] };
	($($e:expr,)*) => { vec![$($e),*] };
	($e:expr; $d:expr) => { vec![$e; $d] };
	($e:expr; $d:expr $(; $ds:expr)+) => { vec![mat![$e $(; $ds)*]; $d] };
}

pub fn get_time() -> f64 {
    static mut STIME: f64 = -1.0;
    let t = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap();
    let ms = t.as_secs() as f64 + t.subsec_nanos() as f64 * 1e-9;
    unsafe {
        if STIME < 0.0 {
            STIME = ms;
        }
        // ローカル環境とジャッジ環境の実行速度差はget_timeで吸収しておくと便利
        #[cfg(feature = "local")]
        {
            (ms - STIME) * 1.0
        }
        #[cfg(not(feature = "local"))]
        {
            ms - STIME
        }
    }
}
