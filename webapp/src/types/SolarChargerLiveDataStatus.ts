import type { ValueObject } from '@/types/LiveDataStatus';

export interface DynamicPowerLimiter {
    PLSTATE: number;
    PLLIMIT: number;
}

export interface SolarCharger {
    full_update: boolean;
    instances: { [key: string]: SolarChargerInstance };
}

interface MpptData {
    [key: string]: ValueObject | string;
}
//type MpptData = (ValueObject | string)[];

export interface SolarChargerInstance {
    data_age_ms: number;
    max_age_ms: number;
    product_id: string;
    firmware_version?: string;
    hide_serial: boolean;
    values: { [key: string]: MpptData };
}
