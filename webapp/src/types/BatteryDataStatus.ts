import type { ValueObject } from '@/types/LiveDataStatus';
import type { StringValue } from '@/types/StringValue';

//type BatteryData = (ValueObject | StringValue)[];

interface BatteryData {
    [key: string]: ValueObject | StringValue;
}

export interface BatteryInstance {
    id: number;
    uid: number;
    name: string;
    enabled: boolean;
    poll_enabled: boolean;
    reachable: boolean;
    producing: boolean;
    limit_absolute: number;
    power: number;
    has_power: boolean;
    manufacturer: string;
    serial: string;
    fwversion: string;
    hwversion: string;
    data_age_ms: number;
    max_age: number;
    values: { [key: string]: BatteryData };
    showIssues: boolean;
    issues: number[];
}

export type Batteries = BatteryInstance[];
