// 设备型号库，用于 tieba 登录取 uid 时的签名数据（对应 baidu_user.h PHONE_MODEL_DATABASE）
// 说明：型号仅作为签名输入的一部分，百度校验 sign 与 data 一致而非型号本身合法性，
// 故此处保留代表性子集即可正常工作（原生版为完整列表）。
export const PHONE_MODELS = [
    "MI 5", "MI 6", "MI 8", "MI MAX", "MIX", "Redmi Note 4", "Redmi Note 5",
    "SM-G9500", "SM-G9550", "SM-G9600", "SM-N9500", "SM-G960F", "SM-G965F",
    "HUAWEI NXT-AL10", "EVA-AL00", "MHA-AL00", "VIE-AL10", "LON-AL00",
    "OPPO R11", "OPPO R9s", "vivo X9", "vivo X20", "vivo X20A",
    "ONEPLUS A5010", "ONE A2001", "Nexus 5X", "Nexus 6P", "Pixel", "Pixel XL",
    "m3 note", "MX6", "Pro 6", "S3",
];
