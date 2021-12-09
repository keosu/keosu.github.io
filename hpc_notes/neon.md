# Neon简介

NEON指令集是ARM处理器支持SIMD的扩展。   
利用NEON来加速运算，通常有几种选择：使用汇编，使用C内嵌汇编，使用intrinsic指令，依赖编译器自动生成向量化指令。
其中使用intrinsic是一种相对开发效率和运行效率都比较高效的方式。

# Neon intrinsic基础

## NEON 的数据类型
基本数据类型格式 ```<type><size>x<number_of_lanes>_t```

> 例如  
> int16x4_t : 代表4个16bit的数据，也就是4个short  
> float32x4_t: 代表4个32bit的float数据   

数组类型 ```<type><size>x<number_of_lanes>x<length_of_array>_t ```

> 例如：  
> struct int16x4x2_t{  
>      int16x4_t val[2];  
> } <var_name>;  
 
 
## neon指令分类

- 正常指令(q): 对上述任意向量类型执行运算，并生成大小相同且类型通常与操作数向量相同的结果向量。

- 长指令(l):  对双字向量操作数执行运算生成四字向量结果。所生成的元素通常是操作数宽度的两倍，并属于同一类型。

- 宽指令(w): 对一个双字向量操作数和一个四字向量操作数执行运算，生成四字向量结果。 所生成的元素和第一个操作数的元素是第二个操作数元素宽度的两倍。

- 窄指令(n): 对四字向量操作数执行运算生成双字向量结果。 所生成的元素通常是操作数元素宽度的一半。

- 饱和指令(q)：当超过数据类型指定的范围则自动限制在该范围内

 
## neon的寄存器
有16个128位寄存器Q0-Q15(或32个64位寄存器D0-D31)

# arm_neon.h主要函数分类

函数的基本命名规范如下：
```
v<q><ops><type>_<flag>_<dtype>

q: <饱和操作>
ops : 具体操作
type: 指令类型 q,l,w,n
flag: n,lane,high or low
dtype: 返回值类型或参数类型

```

arm_neon.h支持的操作

- 初始化，内存加载，存储到内存
- 类型转换
- 数据操作：取某个通道，设置某个通道，重排
- 位操作：移位，位运算
- 算术操作：加减乘除，乘加，乘减，倒数，平方根倒数，取整，最大最小，绝对值

# Neon编程实例

```c
// 向量点积 A⋅B =a1b1+a2b2+a3b3+...anbn
float dot(float* A,float* B,int K) {
    float sum=0;
    float32x4_t sum_vec=vdupq_n_f32(0),left_vec,right_vec;
    for(int k=0;k<K;k+=4) {
        left_vec=vld1q_f32(A+ k);
        right_vec=vld1q_f32(B+ k);
        sum_vec=vmlaq_f32(sum_vec,left_vec,right_vec);
    }

    float32x2_t r=vadd_f32(vget_high_f32(sum_vec),vget_low_f32(sum_vec));
    sum+=vget_lane_f32(vpadd_f32(r,r),0);

    return sum;
}
//  a fast exp method
inline float fast_exp(float x) {
    union {uint32_t i;float f;} v;
    v.i=(1<<23)*(1.4426950409*x+126.93490512f);

    return v.f;
}

inline float expf_sum(float* score,int len)
{
    float sum=0;
    float32x4_t sum_vec=vdupq_n_f32(0);
    float32x4_t ai=vdupq_n_f32(1064807160.56887296), bi;
    int32x4_t   int_vec;
    int value;
    for(int i=0;i<len;i+=4)
    {
        bi=vld1q_f32(score+4*i);
        sum_vec=vmlaq_n_f32(ai,bi,12102203.1616540672);
        int_vec=vcvtq_s32_f32(sum_vec);

        value=vgetq_lane_s32(int_vec,0);
        sum+=(*(float*)(&value));
        value=vgetq_lane_s32(int_vec,1);
        sum+=(*(float*)(&value));
        value=vgetq_lane_s32(int_vec,2);
        sum+=(*(float*)(&value));
        value=vgetq_lane_s32(int_vec,3);
        sum+=(*(float*)(&value));
    }

    return sum;
}
```

# 附录
[Neon指令参考](https://developer.arm.com/architectures/instruction-sets/intrinsics/#f:@navigationhierarchiessimdisa=[Neon])