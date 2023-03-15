NOTE:
These scripts use a recent version of Python, at least > 3.8, ideally 3.10.

Make use of pyenv, pipenv and pyenv-virtualenvs to run successfully
https://github.com/pyenv/pyenv.git 
https://pypi.org/project/pipenv/
https://github.com/pyenv/pyenv-virtualenv.git

Prerequsites
---
Instruction on pyenv (for Linux)
```bash
sudo apt-get install -y make build-essential libssl-dev zlib1g-dev \
libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm libncurses5-dev \
libncursesw5-dev xz-utils tk-dev libffi-dev liblzma-dev python3-openssl
```

install pyenv
---
```bash
git clone https://github.com/pyenv/pyenv.git ~/.pyenv
cd ~/.pyenv && src/configure && make -C src
echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
echo 'command -v pyenv >/dev/null || export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
echo 'eval "$(pyenv init -)"' >> ~/.bashrc
```
The same if you have a .profile
```bash
echo 'export PYENV_ROOT=$HOME/.pyenv' >> ~/.profile
echo 'command -v pyenv >/dev/null || export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.profile
echo 'eval "$(pyenv init -)"' >> ~/.profile
```

**now Restart shell**
```bash
exec bash
```

Install pyenv-virtualenv
```
echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.bashrc
git clone https://github.com/pyenv/pyenv-virtualenv.git $(pyenv root)/plugins/pyenv-virtualenv
```
**now Restart shell**
```bash
exec bash
```
Install a python version
```bash
pyenv install 3.10.10
```

Now, as `.python-version` already exists in this folder and you are using pyenv-virtualenv
when you chdir into this folder python will already be the correct version
