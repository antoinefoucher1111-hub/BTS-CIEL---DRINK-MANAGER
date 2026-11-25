// ==============================================================================
// IMPORTATIONS DES DÉPENDANCES
// ==============================================================================
import { Ionicons, MaterialCommunityIcons } from "@expo/vector-icons"; // Icônes
import { useEffect, useState } from "react"; // Hooks React pour gérer l'état et le cycle de vie
import {
    ActivityIndicator, // Composant pour afficher un loader (roue qui tourne)
    Alert,             // Pour afficher des pop-ups (alertes)
    Platform,          // Pour détecter si on est sur iOS, Android ou Web
    ScrollView,        // Conteneur permettant de faire défiler l'écran
    StyleSheet,        // Pour créer les styles (façon CSS)
    Text,              // Composant pour afficher du texte
    TextInput,         // Champ de saisie de texte
    TouchableOpacity,  // Bouton qui réagit au toucher (devient un peu transparent)
    View,              // Conteneur de base (équivalent d'une <div> en HTML)
} from "react-native";

// Importation de nos propres fichiers (API, Mocks de test, et Types TypeScript)
import { apiClient, mockDrinks, mockUser, USE_MOCK } from "./src/services/api";
import { Drink, UserProfile } from "./src/types";

// ==============================================================================
// CONFIGURATION DU THÈME (COULEURS)
// ==============================================================================
// Palette de couleurs sur le thème du café ☕
const COLORS = {
    background: "#FDF8F5", // Fond beige clair
    primary: "#6F4E37",    // Marron café
    primaryLight: "#8B5A2B", // Marron plus clair
    secondary: "#E6D5C3",  // Beige pour les boutons secondaires/cartes
    textDark: "#3E2723",   // Texte principal (presque noir)
    textMuted: "#8D6E63",  // Texte secondaire (gris-marron)
    white: "#FFFFFF",      // Blanc
    success: "#2E8B57",    // Vert pour les succès
    danger: "#D32F2F",     // Rouge pour les erreurs/soldes bas
    border: "#EFEBE9",     // Couleur des bordures
};

// ==============================================================================
// COMPOSANT PRINCIPAL DE L'APPLICATION
// ==============================================================================
export default function App() {
    // --- DÉCLARATION DES ÉTATS (STATE) ---
    // userId : ID de l'utilisateur actuel (par défaut "1")
    const [userId, setUserId] = useState<string>("1");
    // user : Les données complètes de l'utilisateur (nom, solde...)
    const [user, setUser] = useState<UserProfile | null>(null);
    // drinks : Liste des boissons disponibles à l'achat
    const [drinks, setDrinks] = useState<Drink[]>([]);
    // loading : Indique si l'application est en train de charger les données
    const [loading, setLoading] = useState(true);
    // isEditing : Indique si le mode "Modification du profil" est ouvert
    const [isEditing, setIsEditing] = useState(false);
    // editedName : Stocke le nom tapé dans le champ de texte lors de la modification
    const [editedName, setEditedName] = useState("");

    // --- EFFET 1 : LECTURE DE L'URL (SPÉCIFIQUE AU WEB) ---
    // S'exécute une seule fois au démarrage de l'app.
    useEffect(() => {
        // Vérifie si on est sur le web et s'il y a des paramètres dans l'URL
        if (typeof window !== "undefined" && window.location?.search) {
            const params = new URLSearchParams(window.location.search);
            const qId = params.get("user_id"); // Récupère ?user_id=X
            if (qId) setUserId(qId); // Met à jour le state userId avec la valeur de l'URL
        }
    }, []);

    // --- EFFET 2 : CHARGEMENT DES DONNÉES (API OU MOCK) ---
    // S'exécute au démarrage ET à chaque fois que 'userId' change.
    useEffect(() => {
        const loadData = async () => {
            try {
                setLoading(true); // Affiche le loader
                
                if (USE_MOCK) {
                    // MODE TEST : On utilise de fausses données locales pour tester l'UI sans backend
                    setUser({ ...mockUser });
                    setEditedName(mockUser.name);
                    setDrinks(mockDrinks);
                } else {
                    // MODE RÉEL : On fait des requêtes au serveur backend C++
                    // Promise.all permet de lancer les 2 requêtes en même temps pour aller plus vite
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
                setUser(null); // Si erreur, on vide l'utilisateur pour afficher l'écran d'erreur
            } finally {
                setLoading(false); // Cache le loader, que ça ait réussi ou échoué
            }
        };
        if (userId) loadData(); // Lance la fonction uniquement si on a un ID
    }, [userId]);

    // --- FONCTION : SAUVEGARDER LE PROFIL ---
    const handleSaveProfile = async () => {
        try {
            // Si on n'est pas en mode test, on envoie le nouveau nom au serveur
            if (!USE_MOCK) await apiClient.put(`/users/${userId}`, { name: editedName });
            
            // On met à jour l'interface avec le nouveau nom (correction du type any)
            setUser((prev: UserProfile | null) => (prev ? { ...prev, name: editedName } : null));
            setIsEditing(false); // On ferme le formulaire
            Alert.alert("Succès", "Profil mis à jour !");
        } catch {
            Alert.alert("Erreur", "Échec de la modification.");
        }
    };

    // --- FONCTION : COMMANDER UNE BOISSON ---
    const handleOrderDrink = async (drink: Drink) => {
        if (!user) return; // Sécurité : ne rien faire si l'utilisateur n'est pas chargé
        
        // Vérification : l'utilisateur a-t-il assez d'argent ?
        if (user.balance < drink.price) {
            Alert.alert("Solde insuffisant", `Il manque ${(drink.price - user.balance).toFixed(2)} €.`);
            return; // On arrête la fonction ici
        }
        
        try {
            if (USE_MOCK) {
                // Mode test : on soustrait juste le prix localement (correction du type any)
                setUser((prev: UserProfile | null) => (prev ? { ...prev, balance: user.balance - drink.price } : null));
            } else {
                // Mode réel : on dit au serveur qu'on a commandé, il nous renvoie le nouveau solde
                const res = await apiClient.post("/orders", { user_id: Number(userId), drink_id: drink.id });
                setUser((prev: UserProfile | null) => (prev ? { ...prev, balance: res.data.new_balance } : null));
            }
            Alert.alert("Commande prête ☕", `Un ${drink.name} arrive tout de suite !`);
        } catch {
            Alert.alert("Erreur", "Impossible de valider la commande.");
        }
    };

    // --- FONCTION : RECHARGER LE SOLDE ---
    const handleTopUp = async (amount: number) => {
        if (!user) return;
        try {
            if (USE_MOCK) {
                // Mode test : on ajoute juste le montant localement (correction du type any)
                setUser((prev: UserProfile | null) => (prev ? { ...prev, balance: user.balance + amount } : null));
            } else {
                // Mode réel : on dit au serveur qu'on recharge, il renvoie le nouveau solde
                const res = await apiClient.post("/recharge", { user_id: Number(userId), amount });
                setUser((prev: UserProfile | null) => (prev ? { ...prev, balance: res.data.new_balance } : null));
            }
        } catch {
            Alert.alert("Erreur", "Impossible de créditer le compte.");
        }
    };

    // ==============================================================================
    // RENDUS CONDITIONNELS (Ce qu'on affiche avant l'application principale)
    // ==============================================================================
    
    // 1. Si on charge les données, on affiche le loader
    if (loading) {
        return (
            <View style={styles.center}>
                <ActivityIndicator size="large" color={COLORS.primary} />
                <Text style={styles.loadingText}>Préparation du café #{userId}...</Text>
            </View>
        );
    }

    // 2. Si l'utilisateur n'existe pas (mauvais ID ou erreur serveur)
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

    // ==============================================================================
    // RENDU PRINCIPAL DE L'APPLICATION
    // ==============================================================================
    return (
        <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
            
            {/* EN-TÊTE : Bonjour [Nom] + Bouton Profil */}
            <View style={styles.header}>
                <View>
                    <Text style={styles.greeting}>Bonjour,</Text>
                    {/* Affiche seulement le prénom (coupe au premier espace) */}
                    <Text style={styles.headerName}>{user.name.split(" ")[0]} 👋</Text>
                </View>
                {/* Bouton pour ouvrir/fermer l'édition du profil */}
                <TouchableOpacity style={styles.profileIconBtn} onPress={() => setIsEditing(!isEditing)}>
                    <Ionicons name="person" size={20} color={COLORS.primary} />
                </TouchableOpacity>
            </View>

            {/* FORMULAIRE D'ÉDITION DE PROFIL (Conditionnel : s'affiche si isEditing est true) */}
            {isEditing && (
                <View style={[styles.card, styles.editCard]}>
                    <Text style={styles.inputLabel}>Modifier votre nom</Text>
                    <TextInput
                        style={styles.input}
                        value={editedName}
                        onChangeText={setEditedName} // Met à jour le state quand on tape
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

            {/* CARTE : SOLDE ET RECHARGEMENT */}
            <View style={styles.card}>
                <View style={styles.cardHeader}>
                    <Ionicons name="wallet-outline" size={20} color={COLORS.textMuted} />
                    <Text style={styles.cardTitle}>Mon Solde</Text>
                </View>
                {/* Affiche le solde avec 2 décimales. Si < 1€, on applique un style rouge (balanceLow) */}
                <Text style={[styles.balance, user.balance < 1.0 ? styles.balanceLow : null]}>
                    {user.balance.toFixed(2)} <Text style={styles.currency}>€</Text>
                </Text>
                
                <Text style={styles.subSectionTitle}>Recharger</Text>
                {/* BOUTONS DE RECHARGEMENT GÉNÉRÉS DYNAMIQUEMENT (correction du type any) */}
                <View style={styles.topUpRow}>
                    {[5, 10, 20].map((val: number) => (
                        <TouchableOpacity key={val} style={styles.topUpBtn} onPress={() => handleTopUp(val)}>
                            <Text style={styles.topUpBtnText}>+{val} €</Text>
                        </TouchableOpacity>
                    ))}
                </View>
            </View>

            {/* TITRE : LA CARTE (MENU) */}
            <View style={styles.menuHeader}>
                <Text style={styles.sectionTitle}>La Carte</Text>
                <MaterialCommunityIcons name="coffee-outline" size={24} color={COLORS.primary} />
            </View>

            {/* LISTE DES BOISSONS (correction du type any) */}
            <View style={styles.drinksContainer}>
                {drinks.map((d: Drink) => (
                    // Boucle sur le tableau 'drinks' pour créer une carte par boisson
                    <View key={d.id} style={styles.drinkCard}>
                        <View style={styles.drinkInfo}>
                            <View style={styles.drinkIconContainer}>
                                {/* Change l'icône selon si le mot "thé" est dans le nom */}
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
                        {/* Bouton pour commander cette boisson spécifique */}
                        <TouchableOpacity style={styles.orderBtn} onPress={() => handleOrderDrink(d)}>
                            <Ionicons name="add" size={20} color={COLORS.white} />
                        </TouchableOpacity>
                    </View>
                ))}
            </View>
        </ScrollView>
    );
}

// ==============================================================================
// FEUILLE DE STYLE (CSS pour React Native)
// ==============================================================================
const styles = StyleSheet.create({
    container: { flex: 1, backgroundColor: COLORS.background }, // Prend tout l'écran
    scrollContent: {
        padding: 20,
        // Gère l'encoche (notch) sur iOS vs Android
        paddingTop: Platform.OS === "ios" ? 60 : 40,
        maxWidth: 600, // Empêche l'UI d'être trop large sur le web/tablette
        marginHorizontal: "auto", // Centre sur grand écran
        width: "100%",
        paddingBottom: 40,
    },
    // Style pour centrer le loader ou le message d'erreur
    center: { flex: 1, justifyContent: "center", alignItems: "center", backgroundColor: COLORS.background },
    loadingText: { marginTop: 12, color: COLORS.primary, fontSize: 16, fontWeight: "500" },
    
    // --- En-tête ---
    header: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 24 },
    greeting: { fontSize: 16, color: COLORS.textMuted, fontWeight: "500" },
    headerName: { fontSize: 28, fontWeight: "800", color: COLORS.textDark },
    profileIconBtn: { backgroundColor: COLORS.secondary, padding: 12, borderRadius: 50 },
    
    // --- Cartes (Blocs blancs avec ombre) ---
    card: {
        backgroundColor: COLORS.white,
        borderRadius: 20,
        padding: 24,
        marginBottom: 24,
        // Gestion des ombres (iOS)
        shadowColor: COLORS.textDark,
        shadowOffset: { width: 0, height: 4 },
        shadowOpacity: 0.05,
        shadowRadius: 10,
        // Gestion des ombres (Android)
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
    
    // --- Solde ---
    balance: { fontSize: 42, fontWeight: "800", color: COLORS.textDark },
    currency: { fontSize: 24, color: COLORS.textMuted },
    balanceLow: { color: COLORS.danger }, // S'applique quand l'argent manque
    
    // --- Boutons de rechargement ---
    subSectionTitle: { fontSize: 14, fontWeight: "600", color: COLORS.textMuted, marginTop: 24, marginBottom: 12 },
    topUpRow: { flexDirection: "row", gap: 12 },
    topUpBtn: {
        flex: 1, // Chaque bouton prend la même largeur
        backgroundColor: COLORS.background,
        paddingVertical: 12,
        borderRadius: 12,
        alignItems: "center",
        borderWidth: 1,
        borderColor: COLORS.secondary,
    },
    topUpBtnText: { color: COLORS.primary, fontWeight: "700", fontSize: 16 },
    
    // --- Formulaire d'édition ---
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
    
    // --- Liste des boissons ---
    menuHeader: {
        flexDirection: "row",
        alignItems: "center",
        justifyContent: "space-between",
        marginBottom: 16,
        paddingHorizontal: 4,
    },
    sectionTitle: { fontSize: 22, fontWeight: "700", color: COLORS.textDark },
    drinksContainer: { gap: 12 }, // Espace entre chaque boisson
    drinkCard: {
        flexDirection: "row", // Aligne l'icône, le nom et le bouton sur la même ligne
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
        justifyContent: "center", // Centre l'icône "+" verticalement
        alignItems: "center",     // Centre l'icône "+" horizontalement
        borderRadius: 20,         // Rend le bouton parfaitement rond
    },
});