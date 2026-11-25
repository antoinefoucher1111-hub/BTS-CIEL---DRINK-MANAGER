export interface UserProfile {
    id: number;
    name: string;
    balance: number;
    access_print?: number;
}

export interface BiometricAuthResponse {
    status: string;
    code: string;
    serial_command: string;
    user: UserProfile;
}

export interface Drink {
    id: number;
    name: string;
    category: string;
    stock: number;
    price: number;
}

export interface HistoryItem {
    id: number;
    user_id: number;
    drink_id: number;
    drink_name: string;
    quantity: number;
    datetime: string;
    description: string;
}

export interface HealthResponse {
    status: string;
    backend: string;
    database: string;
}

export interface OrderResponse {
    status: string;
    new_balance: number;
    new_stock: number;
}

export interface RechargeResponse {
    status: string;
    new_balance: number;
}
