// src/services/api.ts
import axios from 'axios';
import { UserProfile, Drink } from '../types';

export const USE_MOCK = true; // Passer à false quand le backend C++ tournera

export const API_BASE_URL = 'http://localhost:8080/api';

export const apiClient = axios.create({
  baseURL: API_BASE_URL,
  headers: { 'Content-Type': 'application/json' },
  timeout: 5000,
});

// Données fictives pour le développement local
export let mockUser: UserProfile = {
  id: 1,
  name: 'Erwann',
  balance: 8.40,
  email: 'erwann@entreprise.lan',
};

export const mockDrinks: Drink[] = [
  { id: 1, name: 'Espresso', price: 0.80 },
  { id: 2, name: 'Double Espresso', price: 1.20 },
  { id: 3, name: 'Allongé', price: 0.90 },
  { id: 4, name: 'Cappuccino', price: 1.50 },
  { id: 5, name: 'Thé / Infusion', price: 0.60 },
];
