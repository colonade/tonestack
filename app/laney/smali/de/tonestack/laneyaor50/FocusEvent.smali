.class public Lde/tonestack/laneyaor50/FocusEvent;
.super Ljava/lang/Object;
.implements Ljava/lang/Runnable;

.field private web:Landroid/webkit/WebView;
.field private code:I

.method public constructor <init>(Landroid/webkit/WebView;I)V
    .locals 0
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    iput-object p1, p0, Lde/tonestack/laneyaor50/FocusEvent;->web:Landroid/webkit/WebView;
    iput p2, p0, Lde/tonestack/laneyaor50/FocusEvent;->code:I
    return-void
.end method

.method public run()V
    .locals 4
    new-instance v0, Ljava/lang/StringBuilder;
    invoke-direct {v0}, Ljava/lang/StringBuilder;-><init>()V
    const-string v1, "window.onAndroidAudioFocus&&window.onAndroidAudioFocus("
    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;
    iget v1, p0, Lde/tonestack/laneyaor50/FocusEvent;->code:I
    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;
    const-string v1, ")"
    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;
    invoke-virtual {v0}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;
    move-result-object v2
    iget-object v3, p0, Lde/tonestack/laneyaor50/FocusEvent;->web:Landroid/webkit/WebView;
    const/4 v1, 0x0
    invoke-virtual {v3, v2, v1}, Landroid/webkit/WebView;->evaluateJavascript(Ljava/lang/String;Landroid/webkit/ValueCallback;)V
    return-void
.end method
