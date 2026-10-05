.class public Lde/tonestack/yamahab100/Chrome;
.super Landroid/webkit/WebChromeClient;


# instance fields
.field private act:Lde/tonestack/yamahab100/MainActivity;


# direct methods
.method public constructor <init>(Lde/tonestack/yamahab100/MainActivity;)V
    .locals 0

    invoke-direct {p0}, Landroid/webkit/WebChromeClient;-><init>()V

    iput-object p1, p0, Lde/tonestack/yamahab100/Chrome;->act:Lde/tonestack/yamahab100/MainActivity;

    return-void
.end method


# virtual methods
.method public onShowFileChooser(Landroid/webkit/WebView;Landroid/webkit/ValueCallback;Landroid/webkit/WebChromeClient$FileChooserParams;)Z
    .locals 4

    iget-object v0, p0, Lde/tonestack/yamahab100/Chrome;->act:Lde/tonestack/yamahab100/MainActivity;

    iget-object v1, v0, Lde/tonestack/yamahab100/MainActivity;->fileCb:Landroid/webkit/ValueCallback;

    if-eqz v1, :cond_0

    const/4 v2, 0x0

    invoke-interface {v1, v2}, Landroid/webkit/ValueCallback;->onReceiveValue(Ljava/lang/Object;)V

    :cond_0
    iput-object p2, v0, Lde/tonestack/yamahab100/MainActivity;->fileCb:Landroid/webkit/ValueCallback;

    invoke-virtual {p3}, Landroid/webkit/WebChromeClient$FileChooserParams;->createIntent()Landroid/content/Intent;

    move-result-object v1

    const/4 v2, 0x1

    :try_start_0
    invoke-virtual {v0, v1, v2}, Landroid/app/Activity;->startActivityForResult(Landroid/content/Intent;I)V
    :try_end_0
    .catch Landroid/content/ActivityNotFoundException; {:try_start_0 .. :try_end_0} :catch_0

    const/4 v3, 0x1

    return v3

    :catch_0
    const/4 v2, 0x0

    iput-object v2, v0, Lde/tonestack/yamahab100/MainActivity;->fileCb:Landroid/webkit/ValueCallback;

    const/4 v3, 0x0

    return v3
.end method
