#include <iostream>
#include <stdexcept>
#include <string>
#include <format> // C++20
#include <cstdint>

enum RegType { W_REG, X_REG, S_REG, D_REG };

std::string common_encode(uint32_t inst) {
    // 假设这是一个通用编码函数，返回编码后的字符串
    return std::to_string(inst); // 简化处理
}

template <typename... Args>
void logInstruction(const std::string& formatStr, Args... args) {
    std::string message = std::vformat(formatStr, std::make_format_args(args...));
    std::cout << std::format("Emit: {} | {}", __func__, message) << std::endl;
}

std::string encodeMul(RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, bool smallEndian = true) {
    std::string reg_char = (regType == X_REG) ? "x" : "w";
    uint32_t inst = 0b00011011000000000111110000000000;
    if (regType == X_REG) {
        inst |= (1 << 31);
    }
    if (rm > 31) {
        throw std::out_of_range("Rm register out of range.");
    }
    inst |= ((rm & 0x1F) << 16);
    if (rn > 31) {
        throw std::out_of_range("Rn register out of range.");
    }
    inst |= ((rn & 0x1F) << 5);
    if (rd > 31) {
        throw std::out_of_range("Rd register out of range.");
    }
    inst |= (rd & 0x1F);
    if (smallEndian) {
        inst = __builtin_bswap32(inst); // convert to small endian
    }
    std::string instruction = common_encode(inst);
    
    logInstruction("{}{}{}, {}{}, {}{}", reg_char, rd, reg_char, rn, reg_char, rm, instruction);
    
    return instruction;
}

std::string encodeMovSP(RegType regType, uint8_t rd, uint8_t rn, bool smallEndian = true) {
    std::string reg_char = (regType == X_REG) ? "x" : "w";
    std::string instruction = common_encode(0); // 简化处理
    std::string rd_str = (rd == 31) ? "sp" : reg_char + std::to_string(rd);
    std::string rn_str = (rn == 31) ? "sp" : reg_char + std::to_string(rn);
    
    logInstruction("mov {}, {} | {}", rd_str, rn_str, instruction);
    
    return instruction;
}

int main() {
    encodeMul(W_REG, 1, 2, 3);
    encodeMovSP(X_REG, 31, 2);
    return 0;
}