import argparse
import subprocess
import shutil
import json
import os
from colorama import Fore, Style
from datetime import date, datetime


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["run", "build", "test", "configure", "workflow", "bench"]
                        , help="run: build and run\nbuild: build\ntest: build and test\nconfigure: configure cmake and "
                               "vcpkg\nworkflow: configure build and test"
                        , default="configure")
    parser.add_argument("-bt", "--build_type", choices=["debug", "relwithdebinfo", "release"], default="debug")
    parser.add_argument("-san", action="store_true", default=False)
    parser.add_argument("-c", "--compiler", choices=["clang", "gcc", "msvc"], default="clang")
    parser.add_argument("-cb", "--create_baseline_bench", action="store_true")
    parser.add_argument("-p", "--platform", choices=["windows", "linux"], default="linux")
    args = parser.parse_args()
    
    if args.compiler == "msvc" and args.platform != "windows":
        parser.error("msvc compiler is only supported on windows")

    if args.san and args.platform == "windows":
        parser.error("\'--san\' is not supported on windows")

    if args.create_baseline_bench and not args.mode == "bench":
        parser.error("\'--create_baseline_bench\' requires the mode to be \'bench\'")

    if args.mode == "bench" and not args.build_type == "release":
        parser.error("mode \'bench\' requires the build type to be \'release\'")
        
    if args.mode == "bench" and args.san:
        parser.error("mode \'bench\' cant be ran with \'san\'")

    return args


def cmake_build(build_type, compiler, platform, san):
    b = build_type
    if san:
        b = build_type + "-sanitize"
    subprocess.run((
        f"cmake --build --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def cmake_test(build_type, compiler, platform, san):
    b = build_type
    if san:
        b = build_type + "-sanitize"
    subprocess.run((
        f"ctest --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def cmake_configure(compiler, platform, san):
    c = compiler
    if san:
        c = compiler + "-sanitize"
    subprocess.run((
        f"cmake --preset=x64-{platform}-{c}"
    ), shell=True)


def cmake_workflow(build_type, compiler, platform, san):
    b = build_type
    if san:
        b = build_type + "-sanitize"
    subprocess.run((
        f"cmake --workflow --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def run(build_type: str, compiler, platform, san):
    c = compiler
    if san:
        c = compiler + "-sanitize"
    
    build_type = build_type.capitalize()
    if platform == "windows":
        subprocess.run((
            f"./build/x64-{platform}-{c}/{build_type}/testbed.exe"
        ))
    else:
        subprocess.run((
            f"./build/x64-{platform}-{c}/{build_type}/testbed"
        ), shell=True)


def do_bench(build_type: str, compiler, platform):
    print(
        f"{Fore.MAGENTA}{Style.BRIGHT}\nRUNNING BENCHMARKS -------------------------------------------------\n{Style.RESET_ALL}")
    if platform == "windows":
        subprocess.run((
            f"./build/x64-{platform}-{compiler}/benches/{build_type}/benches.exe --benchmark_out=benches/results/benches.json --benchmark_out_format=json"
        ))
    else:
        subprocess.run((
            f"./build/x64-{platform}-{compiler}/benches/{build_type}/benches --benchmark_out=benches/results/benches.json --benchmark_out_format=json"
        ), shell=True)


def compare_benches():
    subprocess.run((
        f"py benches/compare.py benchmarks benches/results/baseline.json benches/results/benches.json"
    ), shell=True)


def make_baseline():
    if os.path.exists(f"benches/results/baseline.json"):
        now = datetime.now().strftime("%Y_%m_%d-%I_%M_%S_%p")
        
        if not os.path.exists("benches/results/old_baselines"):
            os.makedirs("benches/results/old_baselines")
            
        shutil.copyfile(f"benches/results/baseline.json",
                        f"benches/results/old_baselines/{now}_baseline.json")

    shutil.copyfile(f"benches/results/benches.json", f"benches/results/baseline.json")


def bench(build_type: str, compiler, platform, create_baseline):
    if not os.path.exists("benches/results"):
        os.makedirs("benches/results")
        
    do_bench(build_type, compiler, platform)
    if os.path.exists(f"benches/results/baseline.json"):
        print()
        compare_benches()
        
    if create_baseline:
        make_baseline()


def copy_compile_commands(compiler, platform, san):
    c = compiler
    if san:
        c = compiler + "-sanitize"
    
    shutil.copyfile(f"build/x64-{platform}-{c}/compile_commands.json", "compile_commands.json")


if __name__ == "__main__":
    args = parse_arguments()
    build_type = args.build_type
    compiler = args.compiler
    platform = args.platform
    san = args.san
    cb = args.create_baseline_bench

    if args.mode == "run":
        cmake_build(build_type, compiler, platform, san)
        run(build_type, compiler, platform, san)
    elif args.mode == "build":
        cmake_build(build_type, compiler, platform, san)
    elif args.mode == "test":
        cmake_build(build_type, compiler, platform, san)
        cmake_test(build_type, compiler, platform, san)
    elif args.mode == "configure":
        cmake_configure(compiler, platform, san)
        if compiler != "msvc":
            copy_compile_commands(compiler, platform, san)
    elif args.mode == "workflow":
        cmake_workflow(build_type, compiler, platform, san)
        if compiler != "msvc":
            copy_compile_commands(compiler, platform, san)
    elif args.mode == "bench":
        cmake_build(build_type, compiler, platform, san)
        bench(build_type, compiler, platform, cb)
