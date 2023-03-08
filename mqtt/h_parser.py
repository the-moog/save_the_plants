import pcpp
from io import StringIO

pp = pcpp.Preprocessor()

trans = "".maketrans({"\r":"", "\n": "", "\"":"", "\'":""})

def get_c_defines(defines, header):
    txt = open(header).read()
    sio = StringIO()

    pp.parse(txt, "dummy.c")
    pp.write(sio)

    return {k: pp.macros[k].value[0].value.translate(trans) for k in defines}

if __name__ == "__main__":
    defines = get_c_defines(("AIO_KEY", "AIO_USERNAME"), "secrets.h")
    print(defines)
