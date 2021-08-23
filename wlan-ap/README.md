# PlumeOS (dev)

| Jenkins | [![Build Status](https://jenkins-m3-hsv.adtran.com/buildStatus/icon?job=SmartRG%2Fwlan-ap%2Fdev)](https://jenkins-m3-hsv.adtran.com/job/SmartRG/job/wlan-ap/job/dev/) |
|:---           |:--- |
| __GitHub__    | https://github.adtran.com/SmartRG/wlan-ap/tree/dev |
| __BitBucket__ | https://bitbucket.org/smartrg/wlan-ap/src/dev/ |

# Building

The `build_SR400ac.sh` automatically invokes Docker, reinvoking itself with a Docker wrapper.  This
mechanism can be bypassed by defining the following environment variable:

    SKIP_DOCKER

For example:

    SKIP_DOCKER=1 ./build_SR400ac.sh

If not provided, the first positional argument defaults to `ENGR`, and the second positional
argument defaults to `build`.  However, if provided, they override the default, like so:

    ./build_SR400ac.sh PROD rebuild

The Docker image is provided by:

* https://github.adtran.com/SmartRG/docker-plumeos-build
* https://jenkins-m3-hsv.adtran.com/job/SmartRG/job/docker-plumeos-build/job/master/

And, it should build all branches of this wlan-ap repo.

The advantage of the Docker image based builds is that the only host requirement is Docker
installation.  All other tooling is encapsulated in the Docker image.

For questions about the Docker wrapper, please email
[Adtran's Developer Experience (DX) team](mailto:FlyingToasters@adtran.com).

# Docker Image Access

For anyone with VPN access to Adtran, the Docker image will be automatically
retrieved upon its first reference from Artifactory.  After the image is pulled,
regardless of method, no further communication to Artifactory is required.  The
image can be manually retrieved using:

    docker pull artifactory.adtran.com/adtran-docker-internal/plumeos-build:11

For those without VPN access, assuming they are able to download a tarball of
the image provided at an alternate download location, they can unpack the image
and load it, like so:

    unxz plumeos-build-11.tar.xz | docker load

For those posting the tarball, it can be created, like so:

    docker pull artifactory.adtran.com/adtran-docker-internal/plumeos-build:11
    docker save artifactory.adtran.com/adtran-docker-internal/plumeos-build:11 -o plumeos-build-11.tar
    xz -9 plumeos-build-11.tar

# Make Options

The default options injected into making openwrt can be overriddent with the
following environment variable:

    MAKE_OPTS

For example:

    MAKE_OPTS='-j1 V=s -Orecurse' ./build_SR400ac.sh ...

It defaults to the following:

    -j$(grep -c processor /proc/cpuinfo) -Orecurse V=s

This will use all available processors, batch Make recipe output per submake
(avoiding one recipe's output interrupting another mid-line), and sets the
verbosity level to `s`.

# Pre-Commit Checks

The build script requires installation of the following Python tool:

[pre-commit](https://pre-commit.com/)

If not installed into the host, it will abort the build and indicate that the
developer should install the package into their host using a command similar to:

    python3 -m pip install pre-commit

The pre-commit checks may be installed into any git repo using the following
command inside the git repo:

    pre-commit install

However, this is not necessary for this repo, because the build scripts will
automatically install the pre-commit hooks, if the pre-commit tool has already
been installed in the host.

Pre-commit checks are executed automatically whenever any commit is attempted.
They only check files staged for commit - not the entire commit history.  The
checks are controlled by the following configuration file:

[/.pre-commit-config.yaml](.pre-commit-config.yaml)

The checks can be manually executed against all files in the repo using:

    pre-commit run --all-files

These checks are verified against all files in the CI/CD (Jenkins) pipeline.

# CI/CD Pipeline (Jenkins)

This repo is periodically synchronized from BitBucket to Adtran's GitHub
Enterprise server by the following GitHub repo and Jenkins job:

* GitHub:  https://github.adtran.com/SmartRG/smartos-bitbucket-reflector
* Jenkins: https://jenkins-m3-hsv.adtran.com/job/SmartRG/job/smartos-bitbucket-reflector/job/master/

The following Jenkins build is triggered any time commits are added to the
wlan-ap GitHub repo:

https://jenkins-m3-hsv.adtran.com/job/SmartRG/job/wlan-ap/job/dev/

The Jenkins build-badge at the top of this page shows the status of the latest
build, and it can be clicked to browse to the Jenkins build page.

The pipeline definition is specified by the presence of the following "marker"
file, which can also be used to override the default job configuration:

[/plumeos-pipeline-config.json](plumeos-pipeline-config.json)

The SmartRG Jenkins organization is
[configured](https://jenkins-m3-hsv.adtran.com/job/SmartRG/configure) to
associate any repo branch with the above file to use the pipeline script defined
by the following repo and file:

GitHub: [plumeos-pipeline](https://github.adtran.com/SmartRG/plumeos-pipeline)/
[plumeos-pipeline.groovy](https://github.adtran.com/SmartRG/plumeos-pipeline/blob/master/plumeos-pipeline.groovy)

# Artifact Storage

As Jenkins builds complete successfully, artifacts are immediately uploaded to:

https://artifactory.adtran.com/artifactory/plumeos-scratch-huntsville/

Artifacts are retained here for a brief period of time.  However, any "released"
artifacts are promoted to the following repo, where they are retained
indefinitely:

https://artifactory.adtran.com/artifactory/plumeos-release-huntsville/

An intermediate "stable" repo was preemptively created to host "scratch"
artifacts that pass automatic testing, but have not yet been promoted to
"release" status.

https://artifactory.adtran.com/artifactory/plumeos-stable-huntsville/

This repo will not be used until automatic testing can be defined and added to
the CI/CD Jenkins pipeline for wlan-ap.

The following "virtual" repo provides a superset of all artifacts from all of
the above repos:

https://artifactory.adtran.com/artifactory/plumeos/

# Setting up your build machine (Non-Docker)

| :exclamation:  Docker based builds should be used instead to eliminate build host variation.  |
|-----------------------------------------------------------------------------------------------|

Requires a recent linux installation. Older systems without python 3.7 will have trouble.  See this link for details: https://openwrt.org/docs/guide-developer/quickstart-build-images

Install build packages:  sudo apt install build-essential libncurses5-dev gawk git libssl-dev gettext zlib1g-dev swig unzip time rsync python3 python3-setuptools python3-yaml.

Plus specific for TIP: sudo apt-get install openvswitch-common

# Doing a native build on Linux
First we need to clone and setup our tree. This will result in an openwrt/.
```
python3 setup.py --setup
```
Next we need to select the profile and base package selection. This setup will install the feeds, packages and generate the .config file. The available profiles are ap2220, ea8300, ecw5211, ecw5410.
```
cd openwrt
./scripts/gen_config.py ap2220 wlan-ap wifi
```
If you want to build an AX image you need to setup a different config. The available profiles are hawkeye, cypress, wallaby.
```
cd openwrt
./scripts/gen_config.py wallaby wifi-ax
```

Finally we can build the tree.
```
make -j X V=s
```
Builds for different profiles can co-exist in the same tree. Switching is done by simple calling gen_config.py again.

# Doing a docker build

Start by installing docker.io on your host system and ensuring that you can run an unprivileged container.
Once this is done edit the Dockerfile and choose the Ubuntu flavour. This might depend on your host installation.
Then simple call (available targets are AP2220, EA8300, ECW5211, ECW5410)
```
TARGET=AP2200 make -j 8
```
