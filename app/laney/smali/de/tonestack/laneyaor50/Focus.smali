.class public Lde/tonestack/laneyaor50/Focus;
.super Ljava/lang/Object;
.implements Landroid/media/AudioManager$OnAudioFocusChangeListener;

# Audio-Fokus für die Wiedergabe: anfordern/abgeben aus der Seite (AndroidAudio.request/abandon), Änderungen an die Seite melden

.field private act:Landroid/app/Activity;
.field private web:Landroid/webkit/WebView;
.field private am:Landroid/media/AudioManager;

.method public constructor <init>(Landroid/app/Activity;Landroid/webkit/WebView;)V
    .locals 1
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    iput-object p1, p0, Lde/tonestack/laneyaor50/Focus;->act:Landroid/app/Activity;
    iput-object p2, p0, Lde/tonestack/laneyaor50/Focus;->web:Landroid/webkit/WebView;
    const-string v0, "audio"
    invoke-virtual {p1, v0}, Landroid/app/Activity;->getSystemService(Ljava/lang/String;)Ljava/lang/Object;
    move-result-object v0
    check-cast v0, Landroid/media/AudioManager;
    iput-object v0, p0, Lde/tonestack/laneyaor50/Focus;->am:Landroid/media/AudioManager;
    return-void
.end method

.method public request()I
    .locals 3
    .annotation runtime Landroid/webkit/JavascriptInterface;
    .end annotation
    iget-object v0, p0, Lde/tonestack/laneyaor50/Focus;->am:Landroid/media/AudioManager;
    if-eqz v0, :cond_0
    const/4 v1, 0x3
    const/4 v2, 0x1
    invoke-virtual {v0, p0, v1, v2}, Landroid/media/AudioManager;->requestAudioFocus(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I
    move-result v0
    return v0
    :cond_0
    const/4 v0, 0x0
    return v0
.end method

.method public abandon()V
    .locals 1
    .annotation runtime Landroid/webkit/JavascriptInterface;
    .end annotation
    iget-object v0, p0, Lde/tonestack/laneyaor50/Focus;->am:Landroid/media/AudioManager;
    if-eqz v0, :cond_0
    invoke-virtual {v0, p0}, Landroid/media/AudioManager;->abandonAudioFocus(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I
    :cond_0
    return-void
.end method

.method public onAudioFocusChange(I)V
    .locals 3
    new-instance v0, Lde/tonestack/laneyaor50/FocusEvent;
    iget-object v1, p0, Lde/tonestack/laneyaor50/Focus;->web:Landroid/webkit/WebView;
    invoke-direct {v0, v1, p1}, Lde/tonestack/laneyaor50/FocusEvent;-><init>(Landroid/webkit/WebView;I)V
    iget-object v2, p0, Lde/tonestack/laneyaor50/Focus;->act:Landroid/app/Activity;
    invoke-virtual {v2, v0}, Landroid/app/Activity;->runOnUiThread(Ljava/lang/Runnable;)V
    return-void
.end method
