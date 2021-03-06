可配置文件：
    inputData.txt文件
        - 此文件是我方输入列表，具体请查看该文件内的说明。
    targetData.txt文件
        - 此文件是目标列表，具体请查看该文件内的说明。

操作说明：
    对话框输入r(高性能)或t(高精准低性能)来运行，q为退出，c为清屏。无需关闭，也可以更改外部配置文件。 

算法解析：
    R模式：
        -根据当前输入的目标，依次计算，每一个目标选择仙人组合中溢出最少的一个。
        -此模式只能保证单层溢出最少，但是所有目标溢出的总和不一定最少。
        -CPU和内存消耗极少。
    T模式：
        -根据当前输入的目标，依次计算，计算所有目标溢出总和最少且能完成的目标总数最多。
        -此模式能保证所有输入的目标的溢出总和最少且完成的目标最多。
        -CPU和内存消耗很多，如果发现计算时间太长，或者内存被用满，请酌情减少输入条目中最高战力仙人，和最低战力仙人的数量。

源码地址: 
    https://github.com/JasonHuang3D/ShangrenTools
    
更新:
    2021.2.3
        -首个版本。
    2021.2.8
        -修改算法，上个版本算法依赖于输入顺序。
        -增加运算错误提示。
        -增加打印统计中剩余没打的仙人。
        -增加只能输入最多64个仙人的限制。
    2021.2.11
        -优化算法，提高了性能。
        -优化输出内容。
        -补充了inputData.txt和targetData.txt的说明。
    2021.2.16
        -增加全局最优解算法：
            1）按t可以使用这个算法。
            2）如果运行时间过长请调整inputData.txt和targetData.txt里的条目数。条目数越多越慢。
        -优化输出内容，增加计算耗时统计。
        -改用Release取代MinSizeRel，从而提高性能。
    2021.2.17
        -v1.5
        -大幅优化T模式性能，inputData.txt尽量不要放小于目标太多的条目数。
    2021.2.18
        -v1.6
        -优化T模式性能，之前会有CPU线程空闲的情况。
    2021.2.27
        -v1.7
        -大幅优化T模式性能，优化内存使用。
        -新增T模式判定，如果当前所有输入仙人的战力总和打不完输入的目标，那么找到能打完的最后的目标为限从第一个目标依次计算。
        -增加了更好的错误处理，防止自动退出。
    2021.3.16
        -v1.7.2
        -使用静态链接库vc runtime。
        -优化多线程。
        -说明文件更改为英文名字。
    2021.4.2
        -v1.7.3
        -优化解析文件，防止因使用了bom后的解析错误。
        -增加工具群联系方式。
    2021.5.2
        -v1.8.0
        -仙人增加"增幅百分比"词条属性，用于计算虚灭防守等。
        -修复加载文件使用tab导致的错误。

联系方式:
email: jasonhuang1988@gmail.com
QQ群: 738202123
