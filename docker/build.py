#!/usr/bin/python3
import argparse
import json
import re
import platform
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--skip-arch', nargs=1, action='append', default=[])
parser.add_argument('--parse-only', action='store_true')
parser.add_argument('--push', action='store_true')
parser.add_argument('cross_file', nargs='+')
args = parser.parse_args()

assert len(args.cross_file) > 0

archs = [
    ('amd64', 'x86_64'),
    ('arm64', 'aarch64')
]

for skip in args.skip_arch:
    for arch in archs:
        if skip[0] in arch:
            archs.remove(arch)
            break

current_platform = platform.machine()


def parse_options(doc):
    # extract options
    options = {}

    for line in doc.splitlines():
        match = re.match(re.compile(r'#:\s*(\S+)\s*=\s*(\S+)\s*?$'), line)
        if match:
            options[match.group(1)] = match.group(2)

    return options


def parse_file(cross_file, arch):
    assert isinstance(arch, tuple)
    with open(cross_file, 'r') as f:
        template = f.read()
    if current_platform == arch[1]:
        # native build
        template = re.sub(re.compile(r'^.*__CROSS_COPY.*$', re.MULTILINE), '', template)
    else:
        template = re.sub(re.compile(r'^(.*)__CROSS_COPY(.*)$', re.MULTILINE), r'\1COPY\2', template)

    template = re.sub(re.compile(r'__BASEIMAGE_ARCH__', re.MULTILINE), arch[0], template)
    template = re.sub(re.compile(r'__QEMU_ARCH__', re.MULTILINE), arch[1], template)

    options = parse_options(template)
    full_name = os.path.splitext(cross_file)
    filename = full_name[0] + "." + arch[0]
    options['filename'] = filename

    with open(filename, 'w') as f:
        f.write(template)
    return options


def main():
    options = []
    for cross in args.cross_file:
        ext = os.path.splitext(cross)[1]
        if ext == ".cross":
            print(f"Parsing .cross file: {cross}")
            for arch in archs:
                print(f"> parsing {arch[0]}")
                options.append(parse_file(cross, arch))
        else:
            print(f"Adding non-cross file: {cross}")
            with open(cross, 'r') as f:
                o = parse_options(f.read())
                o['filename'] = cross
                options.append(o)

        print("===============")

    if args.parse_only:
        sys.exit(0)

    print("Building docker images ...")
    dir_path = os.path.dirname(os.path.realpath(__file__))
    exec_path = os.path.join(dir_path, '..')
    for option in options:
        print(f"Building {option['filename']}")
        ret = os.system(f"docker build {exec_path} -t {option['TAG']} --squash --compress --rm -f {os.path.join(dir_path, option['filename'])}")
        if ret != 0:
            print(f"Building {option['filename']} failed.")
            sys.exit(ret)

    if args.push:
        for option in options:
            print(f"Pushing {option['TAG']}")
            ret = os.system(f"docker push {option['TAG']}")
            if ret != 0:
                print(f"Building {option['filename']} failed.")
                sys.exit(ret)


if __name__ == '__main__':
    main()
