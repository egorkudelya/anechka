from parser.parser import ProtoParser
from generator import Generator


def main():
    parser = ProtoParser()
    IR = parser.parse("proto/proto.txt")
    generator = Generator(IR, "src/network/gen", "src/network/blueprints")
    generator.generate()


if __name__ == "__main__":
    main()
