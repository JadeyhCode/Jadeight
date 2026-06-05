//没错，这本来是一个动画软件，后面我发现这个LLI虚拟机太NB了就单独提了出来
//动画软件版本是0.03，结果回头一看虚拟机已经是1.11了
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
struct TrueEnd:public std::exception{};
struct DataSave {
    uint32_t stackSize;
    uint8_t* Stack;
    uint32_t stackPtr;
    bool end = 0; //end=0是运行，end=1是停止
    //以下为读取
    long long**ptrPtr=nullptr;
    long long commandCode = 0;
    long long parameter = 0/*顾名思义*/;
    unsigned long long count = 0; //刚刚从各个地方转移的
    DataSave():end(false),commandCode(0),parameter(0),count(0){
    }
    DataSave(DataSave&&other) noexcept :count(other.count),end(other.end),commandCode(other.commandCode),parameter(other.parameter){
    }
    DataSave(const DataSave& other):count(other.count),end(other.end),commandCode(other.commandCode),parameter(other.parameter){
    }
    DataSave& operator=(DataSave&&other) noexcept {
        if (this!=&other) {
            this->end=other.end;
            this->commandCode=other.commandCode;
            this->parameter=other.parameter;
            this->count=other.count;
        }
        return *this;
    }
    DataSave&operator=(const DataSave& other) {
        if (this!=&other) {
            DataSave temp(other);
            // 交换当前对象和临时副本的资源
            std::swap(end, temp.end);
            std::swap(commandCode, temp.commandCode);
            std::swap(parameter, temp.parameter);
            std::swap(count, temp.count);
        }else {
            std::cout<<"写一个输出，表示拷贝构造函数发生了意外的自赋值操作，检查一下代码TwT"<<std::endl;
        }
        return *this;
    }
};
struct save{
    std::unique_ptr<uint8_t[]> byteCode;
    size_t size;
    //一想到我还要搞一堆line类的特殊成员函数堆我就头大
    //让我思考一下...
    //是不是要先搞一个构造函数？
    save(const uint8_t *inputBC, size_t size) : size(size),
        byteCode(std::make_unique<uint8_t[]>(size)) {
        if(inputBC!=nullptr){
            for (uint_fast64_t i = 0; i < size; ++i) {
                this->byteCode[i] = inputBC[i];
            }
        }
    }
    //如果故意不用返回值的话可以在前面加(void)
    [[nodiscard]] size_t getSize() const {
        return size;
    }
    //如果故意不用返回值的话可以在前面加(void)
    [[nodiscard]] uint8_t getByte(size_t digit) const {
        return byteCode[digit];
    }
    //如果故意不用返回值的话可以在前面加(void)
    [[nodiscard]] bool getBit(size_t digit) const {
        return (byteCode[digit / 8] >> (digit % 8)) & 1/*偏移，刚学的，但是是第一次用*/;
    }
    /*有了之前的经验change函数提前做*/
    void changeByte/*编辑器为什么让我在后面+const？*/(size_t digit, uint8_t byte)/*这东西能加const吗？*/ {
        byteCode[digit] = byte;
    }
    void changeBit/*同上*/(size_t digit, bool bit) {
        byteCode[digit / 8] = (byteCode[digit / 8] & ~(1ULL << (digit % 8))) | (((bit ? 1ULL : 0ULL) << (digit % 8)));
    }
    //从隔壁line类那里复制来的特殊成员函数
    save(const save &other) : size(other.size), byteCode(std::make_unique<uint8_t[]>(other.size)) {
        for (size_t i = 0; i < other.size; ++i) {
            this->byteCode[i] = other.byteCode[i];
        }
    }
    save(save &&other) noexcept/*总感觉会出问题*/: byteCode(std::move(other.byteCode)), size(other.size) {
        other.size = 0; //感觉没必要啊...但是防止调用的人犯蠢还是加吧
    }
    save &operator=(save &&other) noexcept/*还是那句话，感觉会出问题，再看吧...*/ {
        if (this != &other/*自赋值检查，地址检查*/) {
            byteCode.reset();
            size = 0;
            this->size = other.size;
            this->byteCode = std::move(other.byteCode);
            other.size = 0;
        } else {
            std::cout << "写一个输出，表示移动赋值函数发生了意外的自赋值操作，检查一下代码TwT" << std::endl;
        }
        return *this;
    }

    save &operator=(const save &other) {
        if (this != &other) {
            /*this->size=other.size;
            for(digitT i=0;i<other.size;++i) {
                this->xyList[i]=other.xyList[i];
            }*/ //已淘汰
            // 创建一个临时副本
            save temp(other);
            // 交换当前对象和临时副本的资源
            std::swap(byteCode, temp.byteCode);
            std::swap(size, temp.size);
        } else {
            std::cout << "写一个输出，表示拷贝构造函数发生了意外的自赋值操作，检查一下代码TwT" << std::endl;
        }
        return *this;
    }
};
std::map<std::string, long long> instructionMap = {
    {"add", 0},
    {"reduce", 1},
    //MD我写一半会有一大片报错挡着我，必须一次性写完
    {"ride", 2},
    {"excluding", 3},
    {"modulo", 4},
    {"boolOperation", 5},
    {"Equal", 6},
    {"Greater", 7},
    {"less", 8},
    {"jump", 9},
    {"intPush", 10},
    {"doublePush", 11},
    {"boolPush", 12},
    {"JadeightEnd", 13},
    {"paramStore", 14},
    {"heapAdd", 15},
    {"heapRead", 16},
    {"writeHeap", 17},
    {"freeHeap", 18},
    {"stackSwap", 19},
    {"stackSize", 20},
    {"cout", 21},
    {"readCin", 22},
    {"swapType", 23},
    {"popStack", 24},
    {"copyFirstStack", 25},
    {"pushCount",26}
};
void Stackinitialization(long long parameter,DataSave ptr){
    ptr.Stack=new uint8_t[parameter*1024];
}
void (*interpretedPTR[26])(long long,DataSave*){};
struct executoring {
    //数据存储
    DataSave *ptr;
    /*大脑.exe未响应*/
    //搞个指令解析？
    void interpreted(long long commandCode, long long parameter) {
        std::cout << "当前指令" <<commandCode<< "当前参数" <<parameter<< std::endl;
        interpretedPTR[commandCode](parameter,ptr);
    }
    //全名称：Fast8BitFixedLengthRead
    //注意...你确认要调用这个只适合逻辑模块但是最快，最通用的函数吗？
    //Fast path: direct hardware-style fetch loop
    //goto是故意的，如果接受不了去RS版本
    void F8BFLRead(const save &BYSave) {
        try {
        start:
        ptr->commandCode=BYSave.byteCode[ptr->count];
        ptr->parameter=BYSave.byteCode[ptr->count+1];
        interpreted(ptr->commandCode,ptr->parameter);
        ptr->count+=2;goto start;
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    void RS8BFLRead(const save &BYSave) {
        try {
            if(ptr != nullptr&&(!((BYSave.size)&1))) {
                while(!ptr->end&&ptr->count+1<BYSave.size){
                     ptr->commandCode=BYSave.byteCode[ptr->count];
                    ptr->parameter=BYSave.byteCode[ptr->count+1];
                    interpreted(ptr->commandCode,ptr->parameter);
                    ptr->count+=2;
                }
                if (!ptr->end&&ptr->count+2==BYSave.size) {
                    ptr->commandCode=BYSave.byteCode[ptr->count];
                    ptr->parameter=BYSave.byteCode[ptr->count+1];
                    interpreted(ptr->commandCode,ptr->parameter);
                }
            }
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    //全名称：Fast3ByteFixedLengthRead
    //注意...你确认要调用这个只适合逻辑模块但是最快，最通用的函数吗？
    //Fast path: direct hardware-style fetch loop
    //goto是故意的，如果接受不了去RS版本
    void F3BFLRead(const save &BYSave) {
        try {
            start:
            ptr->commandCode=BYSave.byteCode[ptr->count];
            ptr->parameter=(BYSave.byteCode[ptr->count+1]<<8)+BYSave.byteCode[ptr->count+2];
            interpreted(ptr->commandCode,ptr->parameter);
            ptr->count+=3;goto start;
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    void RS3BFLRead(const save &BYSave) {
        try {
            if(ptr != nullptr&&(((BYSave.size)%3)==0)) {
                while ((!ptr->end&&ptr->count+2<BYSave.size))
                {
                    ptr->commandCode=BYSave.byteCode[ptr->count];
                    ptr->parameter=(BYSave.byteCode[ptr->count+1]<<8)+BYSave.byteCode[ptr->count+2];
                    interpreted(ptr->commandCode,ptr->parameter);
                    ptr->count+=3;
                }
                if (!ptr->end&&ptr->count+2==BYSave.size) {
                    ptr->commandCode=BYSave.byteCode[ptr->count];
                    ptr->parameter=(BYSave.byteCode[ptr->count+1]<<8)+BYSave.byteCode[ptr->count+2];
                    interpreted(ptr->commandCode,ptr->parameter);
                }
            }
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    //全名称：Fast4BitVariableLengthRead
    //注意，危险函数谨慎调用
    //Fast path: direct hardware-style fetch loop
    //goto是故意的，如果接受不了去RS版本
    void F8BVLRead(const save &BYSave) {
        try {
            long long* commandCodePTR=&ptr->commandCode;
            long long* parameterPTR=&ptr->parameter;
            long long*PTRList[2]{&ptr->parameter,&ptr->commandCode};
            long long* endPtr;
            long long* nextEndPtr;
            endPtr=(PTRList[BYSave.byteCode[0]>>7]);
            *endPtr=(BYSave.byteCode[0]<<1)>>1;
            nextEndPtr=(PTRList[BYSave.byteCode[1]>>7]);
            ptr->count=1;
            start:
            endPtr=nextEndPtr;
            *endPtr=(*endPtr*128)+((BYSave.byteCode[ptr->count]<<1)>>1);
            nextEndPtr=(PTRList[BYSave.byteCode[ptr->count+1]>>7]);
            if(endPtr!=commandCodePTR&&(endPtr!=nextEndPtr)){
                interpreted(ptr->commandCode,ptr->parameter);
                ptr->commandCode=0;ptr->parameter=0;
            }
            ptr->count++;goto start;
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    void RS8BVLRead(const save &BYSave) {
        try {
        if (ptr != nullptr) {
            long long*PTRList[2]{&ptr->parameter,&ptr->commandCode};
            long long* endPtr;
            long long* nextEndPtr;
            endPtr=(PTRList[BYSave.byteCode[0]>>7]);
            *endPtr=(BYSave.byteCode[0]<<1)>>1;
            nextEndPtr=(PTRList[BYSave.byteCode[1]>>7]);
            ptr->count=1;
            while (!ptr->end&&ptr->count+1<BYSave.size) {
            endPtr=nextEndPtr;
            *endPtr=(*endPtr*128)+((BYSave.byteCode[ptr->count]<<1)>>1);
            nextEndPtr=(PTRList[BYSave.byteCode[ptr->count+1]>>7]);
            if(endPtr!=PTRList[1]&&endPtr!=nextEndPtr){
                interpreted(ptr->commandCode,ptr->parameter);
                ptr->commandCode=0;ptr->parameter=0;
            }
            ptr->count++;}
            if (!ptr->end&&ptr->count+1==BYSave.size) {
                endPtr=nextEndPtr;
                *endPtr=(*endPtr*128)+((BYSave.byteCode[ptr->count]<<1)>>1);
                interpreted(ptr->commandCode,ptr->parameter);
            }
        }
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    //全名称：Fast4BitVariableLengthRead
    //注意，危险函数谨慎调用
    //Fast path: direct hardware-style fetch loop
    //goto是故意的，如果接受不了去RS版本
    void F4BVLRead(const save &BYSave) {
        try {
        long long*PTRList[2]{&ptr->parameter,&ptr->commandCode};
        long long* endPtr;
        long long* nextEndPtr;
        unsigned long long reading;
        endPtr=(PTRList[(BYSave.byteCode[0]&0x80)!=0]);
        *endPtr=BYSave.byteCode[0]&0x70;
        nextEndPtr=(PTRList[(BYSave.byteCode[0]&0x08)!=0]);
        ptr->count=1;
        start:
        endPtr=nextEndPtr;
        *endPtr=(*endPtr*8)+(BYSave.byteCode[ptr->count>>1]&(0b00000111<<(((!((ptr->count)&1))<<2))));
        nextEndPtr=(PTRList[(BYSave.byteCode[(ptr->count+1)>>1]&(1<<(((!((ptr->count+1)&1))<<2)+3)))!=0]);
        if(endPtr!=PTRList[1]&&(endPtr!=nextEndPtr)){
            interpreted(ptr->commandCode,ptr->parameter);
            ptr->commandCode=0;ptr->parameter=0;
        }
        ptr->count++;if (!ptr->end) {goto start;}
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
    //全名称：ReasonablySafe4BitVariableLengthRead
    //注意，相对安全函数谨慎传参
    void RS4BVLRead(const save &BYSave) {
        try {
        if(ptr != nullptr) {
            long long* commandCodePTR=&ptr->commandCode;
            long long* parameterPTR=&ptr->parameter;
            long long* endPtr;
            long long* nextEndPtr;
            endPtr=(BYSave.byteCode[0]&0x80?commandCodePTR:parameterPTR);
            *endPtr=BYSave.byteCode[0]&0x70;
            nextEndPtr=(BYSave.byteCode[0]&0x08?commandCodePTR:parameterPTR);
            ptr->count=1;
            while (!ptr->end&&ptr->count+1<BYSave.size*2) {
                endPtr=nextEndPtr;
                *endPtr=(*endPtr*8)+(BYSave.byteCode[ptr->count>>1]&(0b00000111<<(((!((ptr->count)&1))<<2))));
                nextEndPtr=(BYSave.byteCode[(ptr->count+1)>>1]&(1<<(((!((ptr->count+1)%2))<<2)+3))?commandCodePTR:parameterPTR);
                if(endPtr!=commandCodePTR&&(endPtr!=nextEndPtr)){
                    interpreted(ptr->commandCode,ptr->parameter);
                    ptr->commandCode=0;
                    ptr->parameter=0;
                }
                ptr->count++;
            }
            if (!ptr->end&&ptr->count+1==BYSave.size*2) {
                endPtr=nextEndPtr;
                *endPtr=(*endPtr*8)+(BYSave.byteCode[ptr->count>>1]&0x70);
                if(endPtr!=commandCodePTR&&(endPtr!=nextEndPtr)){
                    interpreted(ptr->commandCode,ptr->parameter);
                }
            }
        }
        }catch (const std::exception& end) {
            std::cout<<"程序退出"<<std::endl;
        }
    }
        //原版读取逻辑
        /*
        //先来一个for循环读取一下？
        size_t BYSave8=BYSave.size * 8;
        for (ptr->count = 0; (ptr->end == 0) && ptr->count < (BYSave8); ptr->count = ptr->count + 3) {
            if (ptr->count + 2 > BYSave8) { break; }
            ptr->bitStack[0] = ((BYSave.byteCode[ptr->count / 8])>>((ptr->count % 8)) &1 );
            ptr->bitStack[1] = ((BYSave.byteCode[(ptr->count+1) / 8])>>(((ptr->count+1) % 8)) &1 );
            ptr->bitStack[2] = ((BYSave.byteCode[(ptr->count+2) / 8])>>(((ptr->count+2) % 8)) &1 );
            //大脑.exe未响应
            //走一步看一步吧
            if (((ptr->bitStack[0] + (ptr->bitStack[1] << 1) + (ptr->bitStack[2] << 2)) > 2)) {
                if (ptr->readState) {
                    ptr->commandCode = ptr->commandCode * 5 + ((ptr->bitStack[0] + (ptr->bitStack[1] << 1) + (ptr->bitStack[2] << 2)) - 3);
                } else {
                    if (ptr->count!=0) {
                        interpreted(ptr->commandCode, ptr->parameter);
                    }
                    if (ptr->end) {
                        break;
                    }
                    ptr->commandCode = 0;
                    ptr->parameter = 0;
                    ptr->readState = true;
                    ptr->commandCode = (ptr->bitStack[0] + (ptr->bitStack[1] << 1) + (ptr->bitStack[2] << 2) - 3);
                }
            } else {
                if (ptr->readState) {
                    ptr->readState = false;
                }
                ptr->parameter = ptr->parameter * 3 + ((ptr->bitStack[0] + (ptr->bitStack[1] << 1) + (ptr->bitStack[2] << 2)));
                //刚刚是直接复制的，差点忘将-3删掉和5改成3了
                if (ptr->commandCode == 36) {
                    ptr->prepareEnd=1;
                }
            }
        }
        if (ptr->prepareEnd) {
            interpreted(ptr->commandCode, ptr->parameter);
        }
        *///废弃设计
};
//待重构的编译，后面会将compileString提出来。并且添加异常安全，但是在上面修修补补是最快的做法
void compileFile(std::string fileName,std::string outputFileName) {
    std::ifstream inFile(fileName);
    if (!inFile.is_open()) {
        std::cout << "错误：无法打开源文件 \"" << fileName << "\"" << std::endl;
        return;
    }
    std::vector<uint8_t> byteCode;
    std::string line;
    int lineNum = 0;
    bool compilationError = false;

    while (std::getline(inFile, line)) {
        lineNum++;
        // 移除行首行尾空白字符
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue; // 空行跳过
        }
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        // 处理注释（#开头）
        if (line[0] == '#') {
            continue;
        }

        // 查找行内注释并截断
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
            // 再次修剪空白
            size_t newStart = line.find_first_not_of(" \t");
            if (newStart == std::string::npos) {
                continue;
            }
            line = line.substr(newStart);
        }

        // 检查是否为有效行
        if (line.empty()) {
            continue;
        }

        // 解析助记符和参数
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            std::cout << "编译错误 (行 " << lineNum << "): 语法错误，缺少冒号分隔符\n";
            compilationError = true;
            break;
        }

        std::string mnemonic = line.substr(0, colonPos);
        std::string paramStr = line.substr(colonPos + 1);

        // 去除助记符和参数两端的空白
        size_t mnemonicEnd = mnemonic.find_last_not_of(" \t");
        if (mnemonicEnd != std::string::npos) {
            mnemonic = mnemonic.substr(0, mnemonicEnd + 1);
        }
        size_t paramStart = paramStr.find_first_not_of(" \t");
        if (paramStart != std::string::npos) {
            paramStr = paramStr.substr(paramStart);
        }

        // 查找指令码
        auto it = instructionMap.find(mnemonic);
        if (it == instructionMap.end()) {
            std::cout << "编译错误 (行 " << lineNum << "): 未知助记符 \"" << mnemonic << "\"\n";
            compilationError = true;
            break;
        }

        long long commandCode = it->second;
        long long parameter = 0;

        // 解析参数
        if (!paramStr.empty()) {
            try {
                size_t pos = 0;
                parameter = std::stoll(paramStr, &pos);
                if (pos != paramStr.length()) {
                    std::cout << "编译错误 (行 " << lineNum << "): 参数包含非法字符\n";
                    compilationError = true;
                    break;
                }
            } catch (const std::exception &e) {
                std::cout << "编译错误 (行 " << lineNum << "): 参数 \"" << paramStr << "\" 无效\n";
                compilationError = true;
                break;
            }
        }

        // 8BFL 协议：直接写入两个字节
        // 检查范围：opcode 和 parameter 都必须在 0-255 之间
        if (commandCode < 0 || commandCode > 255) {
            std::cout << "编译错误 (行 " << lineNum << "): 指令码 " << commandCode << " 超出 8 位范围 (0-255)\n";
            compilationError = true;
            break;
        }
        if (parameter < 0 || parameter > 255) {
            std::cout << "编译错误 (行 " << lineNum << "): 参数 " << parameter << " 超出 8 位范围 (0-255)\n";
            compilationError = true;
            break;
        }

        // 直接写入 opcode 和 parameter
        byteCode.push_back(static_cast<uint8_t>(commandCode));
        byteCode.push_back(static_cast<uint8_t>(parameter));
    }

    if (compilationError) {
        std::cout << "编译中止\n";
        return;
    }

    inFile.close();
    // 写入输出文件
    std::ofstream outFile(outputFileName, std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "错误：无法创建输出文件 \"" << outputFileName << "\"" << std::endl;
        return;
    }
    outFile.write(reinterpret_cast<const char*>(byteCode.data()), byteCode.size());
    outFile.close();
    if (!outFile) {
        std::cout << "错误：写入文件 \"" << outputFileName << "\" 时发生错误" << std::endl;
        return;
    }
    //以下代码选择性注释
    std::cout << "字节码已成功写入文件: " << outputFileName<< " (大小: " << byteCode.size() << " 字节，共 " << byteCode.size()/2 << " 条指令)" << std::endl;
}
void enterFile(std::string fileName) {
    std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);//几刚NO狗
    if (!inFile.is_open()) {
        std::cout << "错误：无法打开文件 \"" << fileName << "\"" << std::endl;
        return;
    }
    // 获取文件大小
    std::streamsize fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg); //将文件指针移回开头
    // 分配内存并读取整个文件
    std::unique_ptr<uint8_t[]> byteCode = std::make_unique<uint8_t[]>(fileSize);
    inFile.read(reinterpret_cast<char*>(byteCode.get()), fileSize);
    inFile.close();
    if (!inFile) {
        std::cout << "错误：读取文件 \"" << fileName << "\" 时发生错误" << std::endl;
        return;
    }
    //创建 save 对象来保存字节码
    save BYSave(byteCode.get(), static_cast<size_t>(fileSize));
    //创建虚拟机数据存储和执行器
    DataSave data;
    executoring executor;
    executor.ptr = &data;
    std::cout << "开始执行字节码..." << std::endl;
    // 启动读取和执行循环
    executor.RS8BFLRead(BYSave);
    std::cout << std::endl << "执行结束。" << std::endl;
}
//这代码太烂了......
int main() {
    std::cout<<"当前版本仅支持8bitFixedLength编码"<<std::endl;
    std::cout<<"请输入要进行的操作（注：1为读取要编译的文件，2为解释编译后的文件，3为解释说明语法）"<<std::endl;
    int deCin;
    std::cin>>deCin;
    switch (deCin) {
        case 1: {
            std::string fileName;
            std::cout << "请输入源文件路径: ";
            std::cin.ignore();
            std::getline(std::cin, fileName);
            // 写入输出文件
            std::cout << "请输入输出文件名: ";
            std::string outputFileName;
            std::getline(std::cin, outputFileName);
            compileFile(fileName, outputFileName);
            break;
        }
        case 2: {
            std::string fileName;
            std::cout << "请输入字节码文件名: ";
            std::cin.ignore(); //清除之前输入操作符留下的换行符
            std::getline(std::cin, fileName);
            enterFile(fileName);
            break;
            //2026年4月10日题词
            //完工！ヾ(≧▽≦*)o
            //ohh let me fall~~~~~
        }
        case 3: {
            std::cout<<"详细请看program.txt，或把程序代码扔给AI";
            break;
        }
        default:
            std::cout << "未知指令：" <<deCin<< std::endl;
            break;
    }
}
