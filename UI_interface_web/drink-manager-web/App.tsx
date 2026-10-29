import { Ionicons, MaterialCommunityIcons } from "@expo/vector-icons";
import { useEffect, useState } from "react";
import {
    ActivityIndicator,
    Alert,
    Platform,
    ScrollView,
    StyleSheet,
    Text,
    TextInput,
    TouchableOpacity,
    View,
} from "react-native";
import { apiClient, mockDrinks, mockUser, USE_MOCK } from "./src/services/api";
import { Drink, UserProfile } from "./src/types";

const COLORS = {
    background: "#FDF8F5",
    primary: "#6F4E37",
    primaryLight: "#8B5A2B",
    secondary: "#E6D5C3",
    textDark: "#3E2723",
    textMuted: "#8D6E63",
    white: "#FFFFFF",
    success: "#2E8B57",
    danger: "#D32F2F",
    border: "#EFEBE9",
};

export default function App() {
    const [userId, setUserId] = useState<string>("1");
    const [user, setUser] = useState<UserProfile | null>(null);
    const [drinks, setDrinks] = useState<Drink[]>([]);
    const [loading, setLoading] = useState(true);
    const [isEditing, setIsEditing] = useState(false);
    const [editedName, setEditedName] = useState("");

    useEffect(() => {
        if (typeof window !== "undefined" && window.location?.search) {
            const params = new URLSearchParams(window.location.search);
            const qId = params.get("user_id");
            if (qId) setUserId(qId);
        }
    }, []);

    useEffect(() => {
        const loadData = async () => {
            try {
                setLoading(true);
                if (USE_MOCK) {
                    setUser({ ...mockUser });
                    setEditedName(mockUser.name);
                    setDrinks(mockDrinks);
                } else {
                    const [userRes, drinksRes] = await Promise.all([
                        apiClient.get<UserProfile>(`/users/${userId}`),
                        apiClient.get<Drink[]>("/drinks"),
                    ]);
                    setUser(userRes.data);
                    setEditedName(userRes.data.name);
                    setDrinks(drinksRes.data);
                }
            } catch (err: any) {
                console.error("Erreur de chargement:", err);
                setUser(null);
            } finally {
                setLoading(false);
            }
        };
        if (userId) loadData();
    }, [userId]);

    const handleSaveProfile = async () => {
        try {
            if (!USE_MOCK) await apiClient.put(`/users/${userId}`, { name: editedName });
            setUser(prev => (prev ? { ...prev, name: editedName } : null));
            setIsEditing(false);
            Alert.alert("Succès", "Profil mis à jour !");
        } catch {
            Alert.alert("Erreur", "Échec de la modification.");
        }
    };

    const handleOrderDrink = async (drink: Drink) => {
        if (!user) return;
        if (user.balance < drink.price) {
            Alert.alert("Solde insuffisant", `Il manque ${(drink.price - user.balance).toFixed(2)} €.`);
            return;
        }
        try {
            if (USE_MOCK) {
                setUser(prev => (prev ? { ...prev, balance: user.balance - drink.price } : null));
            } else {
                const res = await apiClient.post("/orders", { user_id: Number(userId), drink_id: drink.id });
                setUser(prev => (prev ? { ...prev, balance: res.data.new_balance } : null));
            }
            Alert.alert("Commande prête ☕", `Un ${drink.name} arrive tout de suite !`);
        } catch {
            Alert.alert("Erreur", "Impossible de valider la commande.");
        }
    };

    const handleTopUp = async (amount: number) => {
        if (!user) return;
        try {
            if (USE_MOCK) {
                setUser(prev => (prev ? { ...prev, balance: user.balance + amount } : null));
            } else {
                const res = await apiClient.post("/recharge", { user_id: Number(userId), amount });
                setUser(prev => (prev ? { ...prev, balance: res.data.new_balance } : null));
            }
        } catch {
            Alert.alert("Erreur", "Impossible de créditer le compte.");
        }
    };

    if (loading) {
        return (
            <View style={styles.center}>
                <ActivityIndicator size="large" color={COLORS.primary} />
                <Text style={styles.loadingText}>Préparation du café #{userId}...</Text>
            </View>
        );
    }

    if (!user) {
        return (
            <View style={styles.center}>
                <Text style={{ color: COLORS.danger, fontWeight: "700", fontSize: 18 }}>
                    Utilisateur #{userId} non trouvé.
                </Text>
                <Text style={{ color: COLORS.textMuted, marginTop: 8 }}>
                    Vérifiez l'ID dans l'URL (?user_id=1 à 30).
                </Text>
            </View>
        );
    }

    return (
        <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
            <View style={styles.header}>
                <View>
                    <Text style={styles.greeting}>Bonjour,</Text>
                    <Text style={styles.headerName}>{user.name.split(" ")[0]} 👋</Text>
                </View>
                <TouchableOpacity style={styles.profileIconBtn} onPress={() => setIsEditing(!isEditing)}>
                    <Ionicons name="person" size={20} color={COLORS.primary} />
                </TouchableOpacity>
            </View>

            {isEditing && (
                <View style={[styles.card, styles.editCard]}>
                    <Text style={styles.inputLabel}>Modifier votre nom</Text>
                    <TextInput
                        style={styles.input}
                        value={editedName}
                        onChangeText={setEditedName}
                        placeholder="Votre nom complet"
                    />
                    <View style={styles.editActions}>
                        <TouchableOpacity style={styles.btnSecondary} onPress={() => setIsEditing(false)}>
                            <Text style={styles.btnSecondaryText}>Annuler</Text>
                        </TouchableOpacity>
                        <TouchableOpacity style={styles.btnPrimary} onPress={handleSaveProfile}>
                            <Text style={styles.btnPrimaryText}>Enregistrer</Text>
                        </TouchableOpacity>
                    </View>
                </View>
            )}

            <View style={styles.card}>
                <View style={styles.cardHeader}>
                    <Ionicons name="wallet-outline" size={20} color={COLORS.textMuted} />
                    <Text style={styles.cardTitle}>Mon Solde</Text>
                </View>
                <Text style={[styles.balance, user.balance < 1.0 ? styles.balanceLow : null]}>
                    {user.balance.toFixed(2)} <Text style={styles.currency}>€</Text>
                </Text>
                <Text style={styles.subSectionTitle}>Recharger</Text>
                <View style={styles.topUpRow}>
                    {[5, 10, 20].map(val => (
                        <TouchableOpacity key={val} style={styles.topUpBtn} onPress={() => handleTopUp(val)}>
                            <Text style={styles.topUpBtnText}>+{val} €</Text>
                        </TouchableOpacity>
                    ))}
                </View>
            </View>

            <View style={styles.menuHeader}>
                <Text style={styles.sectionTitle}>La Carte</Text>
                <MaterialCommunityIcons name="coffee-outline" size={24} color={COLORS.primary} />
            </View>

            <View style={styles.drinksContainer}>
                {drinks.map(d => (
                    <View key={d.id} style={styles.drinkCard}>
                        <View style={styles.drinkInfo}>
                            <View style={styles.drinkIconContainer}>
                                <MaterialCommunityIcons
                                    name={d.name.toLowerCase().includes("thé") ? "tea" : "coffee"}
                                    size={24}
                                    color={COLORS.primary}
                                />
                            </View>
                            <View>
                                <Text style={styles.drinkName}>{d.name}</Text>
                                <Text style={styles.drinkPrice}>{d.price.toFixed(2)} €</Text>
                            </View>
                        </View>
                        <TouchableOpacity style={styles.orderBtn} onPress={() => handleOrderDrink(d)}>
                            <Ionicons name="add" size={20} color={COLORS.white} />
                        </TouchableOpacity>
                    </View>
                ))}
            </View>
        </ScrollView>
    );
}

const styles = StyleSheet.create({
    container: { flex: 1, backgroundColor: COLORS.background },
    scrollContent: {
        padding: 20,
        paddingTop: Platform.OS === "ios" ? 60 : 40,
        maxWidth: 600,
        marginHorizontal: "auto",
        width: "100%",
        paddingBottom: 40,
    },
    center: { flex: 1, justifyContent: "center", alignItems: "center", backgroundColor: COLORS.background },
    loadingText: { marginTop: 12, color: COLORS.primary, fontSize: 16, fontWeight: "500" },
    header: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 24 },
    greeting: { fontSize: 16, color: COLORS.textMuted, fontWeight: "500" },
    headerName: { fontSize: 28, fontWeight: "800", color: COLORS.textDark },
    profileIconBtn: { backgroundColor: COLORS.secondary, padding: 12, borderRadius: 50 },
    card: {
        backgroundColor: COLORS.white,
        borderRadius: 20,
        padding: 24,
        marginBottom: 24,
        shadowColor: COLORS.textDark,
        shadowOffset: { width: 0, height: 4 },
        shadowOpacity: 0.05,
        shadowRadius: 10,
        elevation: 3,
    },
    cardHeader: { flexDirection: "row", alignItems: "center", marginBottom: 8 },
    cardTitle: {
        fontSize: 14,
        fontWeight: "600",
        color: COLORS.textMuted,
        marginLeft: 8,
        textTransform: "uppercase",
        letterSpacing: 1,
    },
    balance: { fontSize: 42, fontWeight: "800", color: COLORS.textDark },
    currency: { fontSize: 24, color: COLORS.textMuted },
    balanceLow: { color: COLORS.danger },
    subSectionTitle: { fontSize: 14, fontWeight: "600", color: COLORS.textMuted, marginTop: 24, marginBottom: 12 },
    topUpRow: { flexDirection: "row", gap: 12 },
    topUpBtn: {
        flex: 1,
        backgroundColor: COLORS.background,
        paddingVertical: 12,
        borderRadius: 12,
        alignItems: "center",
        borderWidth: 1,
        borderColor: COLORS.secondary,
    },
    topUpBtnText: { color: COLORS.primary, fontWeight: "700", fontSize: 16 },
    editCard: { backgroundColor: COLORS.secondary, borderWidth: 0 },
    inputLabel: { fontSize: 14, color: COLORS.textDark, fontWeight: "600", marginBottom: 8 },
    input: {
        backgroundColor: COLORS.white,
        borderRadius: 12,
        padding: 14,
        marginBottom: 16,
        fontSize: 16,
        color: COLORS.textDark,
    },
    editActions: { flexDirection: "row", justifyContent: "flex-end", gap: 12 },
    btnPrimary: {
        backgroundColor: COLORS.primary,
        paddingVertical: 12,
        paddingHorizontal: 20,
        borderRadius: 12,
        alignItems: "center",
    },
    btnPrimaryText: { color: COLORS.white, fontWeight: "700", fontSize: 14 },
    btnSecondary: { paddingVertical: 12, paddingHorizontal: 16, borderRadius: 12, alignItems: "center" },
    btnSecondaryText: { color: COLORS.textDark, fontWeight: "600" },
    menuHeader: {
        flexDirection: "row",
        alignItems: "center",
        justifyContent: "space-between",
        marginBottom: 16,
        paddingHorizontal: 4,
    },
    sectionTitle: { fontSize: 22, fontWeight: "700", color: COLORS.textDark },
    drinksContainer: { gap: 12 },
    drinkCard: {
        flexDirection: "row",
        justifyContent: "space-between",
        alignItems: "center",
        backgroundColor: COLORS.white,
        padding: 16,
        borderRadius: 16,
        shadowColor: COLORS.textDark,
        shadowOffset: { width: 0, height: 2 },
        shadowOpacity: 0.03,
        shadowRadius: 8,
        elevation: 2,
    },
    drinkInfo: { flexDirection: "row", alignItems: "center" },
    drinkIconContainer: { backgroundColor: COLORS.background, padding: 10, borderRadius: 12, marginRight: 16 },
    drinkName: { fontSize: 16, fontWeight: "600", color: COLORS.textDark, marginBottom: 4 },
    drinkPrice: { fontSize: 15, color: COLORS.primaryLight, fontWeight: "700" },
    orderBtn: {
        backgroundColor: COLORS.primary,
        width: 40,
        height: 40,
        justifyContent: "center",
        alignItems: "center",
        borderRadius: 20,
    },
});
