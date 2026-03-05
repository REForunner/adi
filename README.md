# ADI - Arm Debug Interface Library

一个用于通过SWD接口操作ARM Cortex-M处理器的C语言库。

## 项目结构

```
adi/
├── adi/          # ADI核心模块 (Debug Port/Access Port接口)
├── processor/    # 处理器控制模块 (Cortex-M核心操作)
├── memory/       # 内存管理模块
├── port/         # 端口抽象层
├── CMakeLists.txt
└── example.c     # 使用示例
```

## 主要功能

### ADI模块
- SWD接口通信
- 调试端口(DP)寄存器读写
- 访问端口(AP)寄存器读写
- 内存读写操作 (8/16/32位)
- 块数据传输
- 内存填充
- 推送验证/比较
- 内存查找
- 轮询等待

### Processor模块
- 调试启用/禁用
- 处理器状态查询
- 核心暂停/运行
- 软件/硬件复位
- 核心寄存器读写

### Memory模块
- 内存镜像管理
- 数据读写

## 使用示例

见 `example.c` 文件，基本流程：

1. 实现SWD接口的回调函数
2. 创建ADI实例
3. 创建Processor实例
4. 连接目标设备
5. 进行调试操作
6. 断开连接并销毁实例

## 注意事项

1. 发生错误时，程序不会写入abort寄存器清除错误标志
2. 使用推送验证或比较时，不允许AP读取
3. 推送验证：值匹配时，STICKYCMP标志设为1
4. 推送比较：值不匹配时，STICKYCMP标志设为1
5. 需要读写idcode/select/ctrlstatus/csw寄存器时，请更新`ADI.last`结构体

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
