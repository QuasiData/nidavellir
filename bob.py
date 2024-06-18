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
    parser.add_argument("-asan", action="store_true", default=False)
    parser.add_argument("-c", "--compiler", choices=["clang", "gcc", "msvc"], default="clang")
    parser.add_argument("-cb", "--create_baseline_bench", action="store_true")
    parser.add_argument("-p", "--platform", choices=["windows", "linux"], default="linux")
    args = parser.parse_args()
    
    if args.compiler == "msvc" and args.platform != "windows":
        parser.error("msvc compiler is only supported on windows")

    if args.asan and args.platform == "windows":
        parser.error("\'--asan\' is not supported on windows")

    if args.create_baseline_bench and not args.mode == "bench":
        parser.error("\'--create_baseline_bench\' requires the mode to be \'bench\'")

    if args.mode == "bench" and not args.build_type == "release":
        parser.error("mode \'bench\' requires the build type to be \'release\'")
        
    if args.mode == "bench" and args.asan:
        parser.error("mode \'bench\' cant be ran with \'asan\'")

    return args


def cmake_build(build_type, compiler, platform, asan):
    b = build_type
    if asan:
        b = build_type + "-asan"
    subprocess.run((
        f"cmake --build --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def cmake_test(build_type, compiler, platform, asan):
    b = build_type
    if asan:
        b = build_type + "-asan"
    subprocess.run((
        f"ctest --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def cmake_configure(compiler, platform, asan):
    c = compiler
    if asan:
        c = compiler + "-asan"
    subprocess.run((
        f"cmake --preset=x64-{platform}-{c}"
    ), shell=True)


def cmake_workflow(build_type, compiler, platform, asan):
    b = build_type
    if asan:
        b = build_type + "-asan"
    subprocess.run((
        f"cmake --workflow --preset=x64-{platform}-{compiler}-{b}"
    ), shell=True)


def run(build_type: str, compiler, platform, asan):
    c = compiler
    if asan:
        c = compiler + "-asan"
    
    build_type = build_type.capitalize()
    if platform == "windows":
        subprocess.run((
            f"./build/x64-{platform}-{c}/{build_type}/testbed.exe"
        ))
    else:
        subprocess.run((
            f"./build/x64-{platform}-{c}/{build_type}/testbed"
        ), shell=True)


def do_bench(build_type: str, compiler, platform, project):
    print(
        f"{Fore.MAGENTA}{Style.BRIGHT}\nRUNNING BENCHMARKS FOR {project} -------------------------------------------------\n{Style.RESET_ALL}")
    if platform == "windows":
        subprocess.run((
            f"./build/x64-{platform}-{compiler}/benches/{project}/{build_type}/{project}_benches.exe --benchmark_out=benches/results/{project}_benches.json --benchmark_out_format=json"
        ))
    else:
        subprocess.run((
            f"./build/x64-{platform}-{compiler}/benches/{project}/{build_type}/{project}_benches --benchmark_out=benches/results/{project}_benches.json --benchmark_out_format=json"
        ), shell=True)


def compare_benches(project):
    subprocess.run((
        f"py benches/compare.py benchmarks benches/results/{project}_baseline.json benches/results/{project}_benches.json"
    ), shell=True)


def make_baseline(project):
    if os.path.exists(f"benches/results/{project}_baseline.json"):
        now = datetime.now().strftime("%Y_%m_%d-%I_%M_%S_%p")
        shutil.copyfile(f"benches/results/{project}_baseline.json",
                        f"benches/results/old_baselines/{now}_{project}_baseline.json")

    shutil.copyfile(f"benches/results/{project}_benches.json", f"benches/results/{project}_baseline.json")


def bench(build_type: str, compiler, platform, project, create_baseline):
    dirs = os.listdir(f"./build/x64-{platform}-{compiler}/benches/")
    if project == "all":
        for dir in dirs:
            if dir != "CMakeFiles" and os.path.isdir(dir):
                do_bench(build_type, compiler, platform, dir)
                if os.path.exists(f"benches/results/{dir}_baseline.json"):
                    print()
                    compare_benches(dir)
                if create_baseline:
                    make_baseline(dir)
    else:
        if project in dirs:
            do_bench(build_type, compiler, platform, project)
            if os.path.exists(f"benches/results/{project}_baseline.json"):
                print()
                compare_benches(project)
            if create_baseline:
                make_baseline(project)
        else:
            print(
                f"{Fore.RED}Error:{Style.RESET_ALL} Tried running benches for project: {project} but it does not exist. Run cmake configure or check spelling.")


def copy_compile_commands(compiler, platform, asan):
    c = compiler
    if asan:
        c = compiler + "-asan"
    
    shutil.copyfile(f"build/x64-{platform}-{c}/compile_commands.json", "compile_commands.json")


if __name__ == "__main__":
    args = parse_arguments()
    build_type = args.build_type
    compiler = args.compiler
    platform = args.platform
    project = "nidavellir"
    asan = args.asan
    cb = args.create_baseline_bench

    if args.mode == "run":
        cmake_build(build_type, compiler, platform, asan)
        run(build_type, compiler, platform, asan)
    elif args.mode == "build":
        cmake_build(build_type, compiler, platform, asan)
    elif args.mode == "test":
        cmake_build(build_type, compiler, platform, asan)
        cmake_test(build_type, compiler, platform, asan)
    elif args.mode == "configure":
        cmake_configure(compiler, platform, asan)
        if compiler != "msvc":
            copy_compile_commands(compiler, platform, asan)
    elif args.mode == "workflow":
        cmake_workflow(build_type, compiler, platform, asan)
        if compiler != "msvc":
            copy_compile_commands(compiler, platform, asan)
    elif args.mode == "bench":
        cmake_build(build_type, compiler, platform, asan)
        bench(build_type, compiler, platform, project, cb)
