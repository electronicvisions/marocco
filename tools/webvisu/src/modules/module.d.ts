// declaration file for Emscriptens global Module object
export as namespace Module;

export function onRuntimeInitialized();
// TODO: complete the Marocco class declaration
export class Marocco{
    constructor()
    constructor(path: string)
    static from_file(path: string): Marocco
    properties(hicann: HICANNOnWafer)
    l1_properties()
}
export class HICANNOnWafer_EnumRanged_type{
    constructor(index: number)
}
export class HICANNOnWafer{
    constructor(enumRanged: HICANNOnWafer_EnumRanged_type)
    x()
    y()
}